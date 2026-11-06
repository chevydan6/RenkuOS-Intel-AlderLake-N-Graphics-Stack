/* SPDX-License-Identifier: MIT */
#include "DekelHdmiPllProgrammer.h"
#include <stdio.h>

struct Mock {
	uint32 control, hip, phy[16384];
	bool autoPower, autoLock, failNextPhyWrite;
	unsigned phyWrites, failPhyWrite, pauses;
};
static status_t Read(void* p, uint32 offset, uint32* out) {
	Mock& m = *(Mock*)p;
	if (offset == 0x46040) { *out = m.control; return B_OK; }
	if (offset >= 0x169000 && offset < 0x16a000) {
		unsigned bank = (m.hip >> 8) & 15;
		*out = m.phy[(bank * 0x1000 + offset - 0x169000) / 4]; return B_OK;
	}
	return B_BAD_VALUE;
}
static status_t Write(void* p, uint32 offset, uint32 value) {
	Mock& m = *(Mock*)p;
	if (offset == 0x1010a0) { m.hip = value; return B_OK; }
	if (offset == 0x46040) {
		m.control = value & ~((1U << 30) | (1U << 26));
		if (m.autoPower && (value & (1U << 27))) m.control |= 1U << 26;
		if (m.autoLock && (value & (1U << 31))) m.control |= 1U << 30;
		return B_OK;
	}
	if (offset >= 0x169000 && offset < 0x16a000) {
		m.phyWrites++;
		if (m.failNextPhyWrite || (m.failPhyWrite && m.phyWrites == m.failPhyWrite)) {
			m.failNextPhyWrite = false; return B_ERROR;
		}
		unsigned bank = (m.hip >> 8) & 15;
		m.phy[(bank * 0x1000 + offset - 0x169000) / 4] = value;
		return B_OK;
	}
	return B_BAD_VALUE;
}
static void Pause(void* p) { ((Mock*)p)->pauses++; }
static AlderLakeTypeCIO IO(Mock& m) { AlderLakeTypeCIO io = {&m, Read, Write, Pause}; return io; }
static void Reset(Mock& m) {
	m = {}; m.autoPower = true; m.autoLock = true;
	for (unsigned i = 0; i < 16384; i++) m.phy[i] = 0x12345678;
}
static bool Restored(const Mock& m) {
	for (unsigned i = 0; i < 16384; i++) if (m.phy[i] != 0x12345678) return false;
	return m.control == 0;
}
int main() {
	unsigned errors = 0; DekelHdmiPllPlan plan = {};
	if (BuildDekelHdmiPllPlan(148500, 19200, &plan) != B_OK) return 1;
	Mock m;
	Reset(m);
	{
		DekelHdmiPllProgrammer p(IO(m), 2, 3);
		if (p.Program(plan) != B_OK || p.Program(plan) != B_BUSY
			|| !(m.control & (1U << 30)) || p.Release() != B_OK || !Restored(m)) errors++;
	}
	Reset(m); m.control = 1U << 31;
	{ DekelHdmiPllProgrammer p(IO(m), 2, 3); if (p.Program(plan) != B_BUSY || m.phyWrites) errors++; }
	Reset(m); m.autoPower = false;
	{ DekelHdmiPllProgrammer p(IO(m), 2, 3); if (p.Program(plan) != B_TIMED_OUT || !Restored(m)) errors++; }
	Reset(m); m.autoLock = false;
	{ DekelHdmiPllProgrammer p(IO(m), 2, 3); if (p.Program(plan) != B_TIMED_OUT || !Restored(m)) errors++; }
	for (unsigned fail = 1; fail <= 8; fail++) {
		Reset(m); m.failPhyWrite = fail;
		DekelHdmiPllProgrammer p(IO(m), 2, 3);
		if (p.Program(plan) != B_ERROR || !Restored(m)) errors++;
	}
	Reset(m);
	{
		DekelHdmiPllProgrammer p(IO(m), 2, 3);
		if (p.Program(plan) != B_OK) errors++;
		m.failNextPhyWrite = true;
		if (p.Release() != B_ERROR || p.Program(plan) != B_BUSY
			|| p.Release() != B_OK || !Restored(m)) errors++;
	}
	Reset(m); plan.updates[0].value = 0xffffffffU;
	{ DekelHdmiPllProgrammer p(IO(m), 2, 3); if (p.Program(plan) != B_BAD_VALUE || !Restored(m)) errors++; }
	BuildDekelHdmiPllPlan(148500, 19200, &plan);
	Reset(m); plan.updates[0].internalOffset = 0x2000;
	{ DekelHdmiPllProgrammer p(IO(m), 2, 3); if (p.Program(plan) != B_BAD_VALUE || !Restored(m)) errors++; }
	BuildDekelHdmiPllPlan(148500, 19200, &plan);
	Reset(m); plan.updates[0].mask = 0xffffffffU;
	{ DekelHdmiPllProgrammer p(IO(m), 2, 3); if (p.Program(plan) != B_BAD_VALUE || !Restored(m)) errors++; }
	if (errors) { fprintf(stderr, "%u PLL programmer cases failed\n", errors); return 1; }
	puts("16 Dekel PLL programming, timeout and rollback cases passed; mock registers only");
	return 0;
}
