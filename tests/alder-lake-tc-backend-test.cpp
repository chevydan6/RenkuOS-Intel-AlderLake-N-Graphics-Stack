/* SPDX-License-Identifier: MIT */
#include "AlderLakeTypeCBackend.h"
#include <stdio.h>

struct Mock {
	uint32 power, buffer, status, health, hip;
	unsigned pauses, bufferReads, statusReads, healthReads;
};
static status_t Read(void* p, uint32 address, uint32* out) {
	Mock& m = *(Mock*)p;
	if (address == 0x45444) *out = m.power;
	else if (address == 0x64400) { *out = m.buffer; m.bufferReads++; }
	else if (address == 0x161504) { *out = m.status; m.statusReads++; }
	else if (address == 0x16936c && m.hip == 0x200) { *out = m.health; m.healthReads++; }
	else return B_BAD_VALUE;
	return B_OK;
}
static status_t Write(void* p, uint32 address, uint32 value) {
	Mock& m = *(Mock*)p;
	if (address == 0x45444) {
		m.power = value;
		if (value & (1U << 9)) m.power |= 1U << 8;
		else m.power &= ~(1U << 8);
	} else if (address == 0x64400) m.buffer = value;
	else if (address == 0x1010a0) m.hip = value;
	else return B_BAD_VALUE;
	return B_OK;
}
static void Pause(void* p) { ((Mock*)p)->pauses++; }
static status_t WellRead(void* p, uint32* out) { return Read(p, 0x45444, out); }
static status_t WellWrite(void* p, uint32 value) { return Write(p, 0x45444, value); }
static TypeCPowerWellOps WellOps(Mock& m) {
	TypeCPowerWellOps ops = {&m, WellRead, WellWrite, Pause}; return ops;
}
static AlderLakeTypeCIO IO(Mock& m) {
	AlderLakeTypeCIO io = {&m, Read, Write, Pause}; return io;
}

int main() {
	unsigned errors = 0;
	for (unsigned alreadyOwned = 0; alreadyOwned < 2; alreadyOwned++) {
		Mock m = {0, 0x1200U | (alreadyOwned ? 0x40U : 0), 4, 1U << 15, 0, 0, 0, 0, 0};
		TypeCPowerWell well(WellOps(m), 2, 3);
		AlderLakeTypeCBackend backend(IO(m), well, 2, 3);
		TypeCPowerLifecycle lifecycle(backend.LifecycleOps());
		if (lifecycle.Acquire() != B_OK || !(m.buffer & 0x40)
			|| !well.IsHeld() || m.bufferReads == 0 || m.statusReads != 1 || m.healthReads != 1
			|| lifecycle.Release() != B_OK || well.IsHeld()
			|| m.buffer != (0x1200U | (alreadyOwned ? 0x40U : 0))) errors++;
	}
	{
		Mock m = {0, 0x1200, 0, 1U << 15, 0, 0, 0, 0, 0};
		TypeCPowerWell well(WellOps(m), 2, 3);
		AlderLakeTypeCBackend backend(IO(m), well, 2, 3);
		TypeCPowerLifecycle lifecycle(backend.LifecycleOps());
		if (lifecycle.Acquire() != B_TIMED_OUT || m.statusReads != 3
			|| m.pauses != 3 || well.IsHeld() || m.buffer != 0x1200) errors++;
	}
	{
		Mock m = {0, 0x1200, 0xffffffffU, 1U << 15, 0, 0, 0, 0, 0};
		TypeCPowerWell well(WellOps(m), 2, 3);
		AlderLakeTypeCBackend backend(IO(m), well, 2, 3);
		TypeCPowerLifecycle lifecycle(backend.LifecycleOps());
		if (lifecycle.Acquire() != B_ERROR || well.IsHeld() || m.buffer != 0x1200)
			errors++;
	}
	{
		Mock m = {0, 0x1200, 4, 1U << 15, 0, 0, 0, 0, 0};
		TypeCPowerWell well(WellOps(m), 1, 3);
		AlderLakeTypeCBackend backend(IO(m), well, 2, 3);
		TypeCPowerLifecycle lifecycle(backend.LifecycleOps());
		if (lifecycle.Acquire() != B_BAD_VALUE || well.IsHeld() || m.bufferReads)
			errors++;
	}
	{
		Mock m = {0, 0x1200, 4, 0, 0, 0, 0, 0, 0};
		TypeCPowerWell well(WellOps(m), 2, 3);
		AlderLakeTypeCBackend backend(IO(m), well, 2, 3);
		TypeCPowerLifecycle lifecycle(backend.LifecycleOps());
		if (lifecycle.Acquire() != B_TIMED_OUT || m.healthReads != 3
			|| m.bufferReads != 0 || well.IsHeld()) errors++;
	}
	if (errors) { fprintf(stderr, "%u Alder Lake backend cases failed\n", errors); return 1; }
	puts("6 Alder Lake Type-C backend and rollback cases passed; mock registers only");
	return 0;
}
