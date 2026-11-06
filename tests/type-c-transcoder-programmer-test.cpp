/* SPDX-License-Identifier: MIT */
#include "TypeCHdmiTranscoderProgrammer.h"
#include <stdio.h>

struct Mock { uint32 clock, function; unsigned writes, failWrite; bool badReadback; };
static status_t Read(void* p, uint32 offset, uint32* value) {
	Mock& m = *(Mock*)p;
	if (offset == 0x46144) *value = m.clock;
	else if (offset == 0x61400) *value = m.badReadback && (m.function & 0x80000000U)
		? m.function & ~0x80000000U : m.function;
	else return B_BAD_VALUE;
	return B_OK;
}
static status_t Write(void* p, uint32 offset, uint32 value) {
	Mock& m = *(Mock*)p;
	m.writes++;
	if (m.failWrite && m.writes == m.failWrite) return B_ERROR;
	if (offset == 0x46144) m.clock = value;
	else if (offset == 0x61400) m.function = value;
	else return B_BAD_VALUE;
	return B_OK;
}
static void Pause(void*) {}
static AlderLakeTypeCIO IO(Mock& m) { AlderLakeTypeCIO io = {&m, Read, Write, Pause}; return io; }
static bool Restored(const Mock& m) { return m.clock == 0 && m.function == 0x30000; }
int main() {
	unsigned errors = 0; TypeCHdmiTranscoderPlan plan = {};
	if (BuildAlderLakeMobileTypeCHdmiPlan(2, 8, true, true, &plan) != B_OK) return 1;
	{
		Mock m = {0, 0x30000, 0, 0, false}; TypeCHdmiTranscoderProgrammer p(IO(m), 1);
		if (p.Configure(plan) != B_OK || m.clock != 0x70000000U
			|| p.Enable() != B_OK || m.function != 0xa8030000U
			|| p.Configure(plan) != B_BUSY || p.Release() != B_OK || !Restored(m)) errors++;
	}
	{
		Mock m = {0x10000000, 0x30000, 0, 0, false}; TypeCHdmiTranscoderProgrammer p(IO(m), 1);
		if (p.Configure(plan) != B_BUSY || m.writes != 0) errors++;
	}
	{
		Mock m = {0, 0x80030000U, 0, 0, false}; TypeCHdmiTranscoderProgrammer p(IO(m), 1);
		if (p.Configure(plan) != B_BUSY || m.writes != 0) errors++;
	}
	for (unsigned fail = 1; fail <= 3; fail++) {
		Mock m = {0, 0x30000, 0, fail, false}; TypeCHdmiTranscoderProgrammer p(IO(m), 1);
		status_t result = p.Configure(plan);
		if (result == B_OK) result = p.Enable();
		if (result != B_ERROR || !Restored(m)) errors++;
	}
	{
		Mock m = {0, 0x30000, 0, 0, true}; TypeCHdmiTranscoderProgrammer p(IO(m), 1);
		if (p.Configure(plan) != B_OK || p.Enable() != B_ERROR || !Restored(m)) errors++;
	}
	{
		Mock m = {0, 0x30000, 0, 0, false}; TypeCHdmiTranscoderProgrammer p(IO(m), 1);
		if (p.Configure(plan) != B_OK || p.Enable() != B_OK) errors++;
		m.failWrite = m.writes + 1;
		if (p.Release() != B_ERROR || p.Configure(plan) != B_BUSY) errors++;
		m.failWrite = 0;
		if (p.Release() != B_OK || !Restored(m)) errors++;
	}
	{
		Mock m = {0, 0x30000, 0, 0, false}; TypeCHdmiTranscoderProgrammer p(IO(m), 1);
		plan.clockSelect = 0;
		if (p.Configure(plan) != B_BAD_VALUE || m.writes != 0) errors++;
	}
	if (errors) { fprintf(stderr, "%u transcoder programmer cases failed\n", errors); return 1; }
	puts("9 transcoder programming, conflict and rollback cases passed; mock registers only");
	return 0;
}
