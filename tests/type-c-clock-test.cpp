/* SPDX-License-Identifier: MIT */
#include "TypeCHdmiClock.h"
#include <stdio.h>

struct Mock { uint32 buffer, pll, portClock, gate; unsigned writes, failWrite; bool stickyGate; };
static status_t Read(void* p, uint32 offset, uint32* value) {
	Mock& m = *(Mock*)p;
	if (offset == 0x64400) *value = m.buffer;
	else if (offset == 0x46040) *value = m.pll;
	else if (offset == 0x46110) *value = m.portClock;
	else if (offset == 0x164280) *value = m.gate;
	else return B_BAD_VALUE;
	return B_OK;
}
static status_t Write(void* p, uint32 offset, uint32 value) {
	Mock& m = *(Mock*)p; m.writes++;
	if (m.failWrite && m.writes == m.failWrite) return B_ERROR;
	if (offset == 0x46110) m.portClock = value;
	else if (offset == 0x164280) { if (!m.stickyGate) m.gate = value; }
	else return B_BAD_VALUE;
	return B_OK;
}
static void Pause(void*) {}
static AlderLakeTypeCIO IO(Mock& m) { AlderLakeTypeCIO io = {&m, Read, Write, Pause}; return io; }
static Mock Default() { Mock m = {0x40, 0xc0000000U, 0x123, 0xa5a52000U, 0, 0, false}; return m; }
static bool Restored(const Mock& m) { return m.portClock == 0x123 && m.gate == 0xa5a52000U; }
int main() {
	unsigned errors = 0;
	{
		Mock m = Default(); TypeCHdmiClock clock(IO(m), 2);
		if (clock.Enable() != B_OK || m.portClock != 0x80000123U
			|| (m.gate & 0x2000) || clock.Enable() != B_BUSY
			|| clock.Release() != B_OK || !Restored(m)) errors++;
	}
	for (unsigned fail = 1; fail <= 2; fail++) {
		Mock m = Default(); m.failWrite = fail; TypeCHdmiClock clock(IO(m), 2);
		if (clock.Enable() != B_ERROR || !Restored(m)) errors++;
	}
	{
		Mock m = Default(); m.stickyGate = true; TypeCHdmiClock clock(IO(m), 2);
		if (clock.Enable() != B_ERROR || !Restored(m)) errors++;
	}
	{
		Mock m = Default(); m.pll = 0x80000000U; TypeCHdmiClock clock(IO(m), 2);
		if (clock.Enable() != B_ERROR || m.writes) errors++;
	}
	{
		Mock m = Default(); m.buffer |= 0x80000000U; TypeCHdmiClock clock(IO(m), 2);
		if (clock.Enable() != B_BUSY || m.writes) errors++;
	}
	{
		Mock m = Default(); m.portClock |= 0x10000000U; TypeCHdmiClock clock(IO(m), 2);
		if (clock.Enable() != B_BUSY || m.writes) errors++;
	}
	{
		Mock m = Default(); TypeCHdmiClock clock(IO(m), 2);
		if (clock.Enable() != B_OK) errors++;
		m.failWrite = m.writes + 1;
		if (clock.Release() != B_ERROR || clock.Enable() != B_BUSY) errors++;
		m.failWrite = 0;
		if (clock.Release() != B_OK || !Restored(m)) errors++;
	}
	if (errors) { fprintf(stderr, "%u Type-C clock cases failed\n", errors); return 1; }
	puts("8 Type-C clock ownership, gating and rollback cases passed; mock registers only");
	return 0;
}
