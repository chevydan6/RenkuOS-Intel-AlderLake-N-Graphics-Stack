/* SPDX-License-Identifier: MIT */
#include "DekelHdmiSignalProgrammer.h"
#include <stdio.h>

struct Mock { uint32 buffer, status, hip, phy[16384]; unsigned phyWrites, failWrite; bool failNext; };
static status_t Read(void* p, uint32 offset, uint32* value) {
	Mock& m = *(Mock*)p;
	if (offset == 0x64400) { *value = m.buffer; return B_OK; }
	if (offset == 0x161504) { *value = m.status; return B_OK; }
	if (offset >= 0x169000 && offset < 0x16a000) {
		unsigned bank = (m.hip >> 8) & 15;
		*value = m.phy[(bank * 0x1000 + offset - 0x169000) / 4]; return B_OK;
	}
	return B_BAD_VALUE;
}
static status_t Write(void* p, uint32 offset, uint32 value) {
	Mock& m = *(Mock*)p;
	if (offset == 0x1010a0) { m.hip = value; return B_OK; }
	if (offset >= 0x169000 && offset < 0x16a000) {
		m.phyWrites++;
		if (m.failNext || (m.failWrite && m.phyWrites == m.failWrite)) {
			m.failNext = false; return B_ERROR;
		}
		unsigned bank = (m.hip >> 8) & 15;
		m.phy[(bank * 0x1000 + offset - 0x169000) / 4] = value;
		return B_OK;
	}
	return B_BAD_VALUE;
}
static void Pause(void*) {}
static AlderLakeTypeCIO IO(Mock& m) { AlderLakeTypeCIO io = {&m, Read, Write, Pause}; return io; }
static void Reset(Mock& m) {
	m = {}; m.buffer = 1U << 6; m.status = 1U << 2;
	for (unsigned i = 0; i < 16384; i++) m.phy[i] = 0x12345678;
}
static bool Restored(const Mock& m) {
	for (unsigned i = 0; i < 16384; i++) if (m.phy[i] != 0x12345678) return false;
	return true;
}
int main() {
	unsigned errors = 0; Mock m;
	Reset(m);
	{
		DekelHdmiSignalProgrammer p(IO(m), 2);
		if (p.Apply(9, 148500, false) != B_OK || p.Apply(9, 148500, false) != B_BUSY
			|| p.Restore() != B_OK || !Restored(m)) errors++;
	}
	for (unsigned fail = 1; fail <= 8; fail++) {
		Reset(m); m.failWrite = fail; DekelHdmiSignalProgrammer p(IO(m), 2);
		if (p.Apply(9, 148500, false) != B_ERROR || !Restored(m)) errors++;
	}
	Reset(m); m.buffer |= 1U << 31;
	{ DekelHdmiSignalProgrammer p(IO(m), 2); if (p.Apply(9, 148500, false) != B_BUSY || m.phyWrites) errors++; }
	Reset(m); m.buffer = 0;
	{ DekelHdmiSignalProgrammer p(IO(m), 2); if (p.Apply(9, 148500, false) != B_ERROR || m.phyWrites) errors++; }
	Reset(m); m.status = 0;
	{ DekelHdmiSignalProgrammer p(IO(m), 2); if (p.Apply(9, 148500, false) != B_ERROR || m.phyWrites) errors++; }
	Reset(m);
	{ DekelHdmiSignalProgrammer p(IO(m), 2); if (p.Apply(10, 148500, false) != B_BAD_VALUE || m.phyWrites) errors++; }
	Reset(m);
	{
		DekelHdmiSignalProgrammer p(IO(m), 2);
		if (p.Apply(9, 148500, false) != B_OK) errors++;
		m.buffer |= 1U << 31;
		if (p.Restore() != B_BUSY) errors++;
		m.buffer &= ~(1U << 31); m.failNext = true;
		if (p.Restore() != B_ERROR || p.Restore() != B_OK || !Restored(m)) errors++;
	}
	if (errors) { fprintf(stderr, "%u signal programmer cases failed\n", errors); return 1; }
	puts("14 PHY signal programming, conflict and rollback cases passed; mock registers only");
	return 0;
}
