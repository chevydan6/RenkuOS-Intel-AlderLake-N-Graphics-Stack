/* SPDX-License-Identifier: MIT */
#include "TypeCPowerWell.h"
#include "TypeCPowerLifecycle.h"
#include <stdio.h>

struct MockWell {
	uint32 value;
	bool autoState, failRead, failSet, failClear;
	unsigned pauses, writes;
};
static status_t Read(void* p, uint32* value) {
	MockWell& m = *(MockWell*)p;
	if (m.failRead) return B_ERROR;
	*value = m.value;
	return B_OK;
}
static status_t Write(void* p, uint32 value) {
	MockWell& m = *(MockWell*)p;
	const uint32 request = 1U << 9; // TC2
	if (((value & request) && m.failSet)
		|| (!(value & request) && m.failClear)) return B_ERROR;
	m.value = value; m.writes++;
	return B_OK;
}
static void Pause(void* p) {
	MockWell& m = *(MockWell*)p;
	m.pauses++;
	if (m.autoState) m.value |= 1U << 8;
}
static TypeCPowerWellOps Ops(MockWell& m) {
	TypeCPowerWellOps ops = {&m, Read, Write, Pause}; return ops;
}
struct Combined { TypeCPowerWell* well; bool owned; };
static status_t AcquireCombined(void* p, bool* held) {
	Combined& c = *(Combined*)p;
	status_t status = c.well->Acquire(); *held = c.well->IsHeld(); return status;
}
static status_t ReleaseCombined(void* p) { return ((Combined*)p)->well->Release(); }
static status_t OwnedCombined(void* p, bool* out) { *out = ((Combined*)p)->owned; return B_OK; }
static status_t SetCombined(void* p, bool owned) { ((Combined*)p)->owned = owned; return B_OK; }
static status_t ReadyCombined(void*, bool* ready) { *ready = true; return B_OK; }
int main() {
	unsigned errors = 0;
	{
		MockWell m = {0x40000000, true, false, false, false, 0, 0};
		TypeCPowerWell well(Ops(m), 2, 4);
		if (well.Acquire() != B_OK || m.pauses != 1 || well.Acquire() != B_OK
			|| well.Release() != B_OK || (m.value & (1U << 9)) == 0
			|| well.Release() != B_OK || (m.value & (1U << 9)) != 0
			|| (m.value & 0x40000000) == 0 || m.writes != 2) errors++;
	}
	{
		MockWell m = {(1U << 9) | (1U << 8), false, false, false, false, 0, 0};
		TypeCPowerWell well(Ops(m), 2, 4);
		if (well.Acquire() != B_OK || well.Release() != B_OK
			|| !(m.value & (1U << 9)) || m.writes != 0) errors++;
	}
	{
		MockWell m = {0, false, false, false, false, 0, 0};
		TypeCPowerWell well(Ops(m), 2, 3);
		if (well.Acquire() != B_TIMED_OUT || m.pauses != 3
			|| (m.value & (1U << 9)) != 0 || m.writes != 2) errors++;
	}
	{
		MockWell m = {0, false, false, false, true, 0, 0};
		TypeCPowerWell well(Ops(m), 2, 2);
		if (well.Acquire() != B_ERROR || well.Acquire() != B_BUSY
			|| !(m.value & (1U << 9))) errors++;
		m.failClear = false;
		if (well.Release() != B_OK || (m.value & (1U << 9))) errors++;
	}
	{
		MockWell m = {0, false, true, false, false, 0, 0};
		TypeCPowerWell well(Ops(m), 2, 2);
		if (well.Acquire() != B_ERROR || m.writes != 0) errors++;
	}
	{
		MockWell m = {0, false, false, true, false, 0, 0};
		TypeCPowerWell well(Ops(m), 2, 2);
		if (well.Acquire() != B_ERROR || m.writes != 0) errors++;
	}
	{
		MockWell m = {0xffffffffU, false, false, false, false, 0, 0};
		TypeCPowerWell well(Ops(m), 2, 2);
		if (well.Acquire() != B_ERROR || m.writes != 0) errors++;
	}
	{
		MockWell m = {0, false, false, false, false, 0, 0};
		TypeCPowerWell invalidPort(Ops(m), 0, 2), invalidTries(Ops(m), 2, 0);
		if (invalidPort.Acquire() != B_BAD_VALUE
			|| invalidTries.Acquire() != B_BAD_VALUE || m.writes != 0) errors++;
	}
	for (unsigned port = 1; port <= 4; port++) {
		uint32 state = 1U << (2 * (port + 2));
		uint32 request = 2U << (2 * (port + 2));
		MockWell m = {state | request, false, false, false, false, 0, 0};
		TypeCPowerWell well(Ops(m), port, 2);
		if (well.Acquire() != B_OK || well.Release() != B_OK
			|| m.value != (state | request) || m.writes != 0) errors++;
	}
	{
		MockWell m = {0, true, false, false, false, 0, 0};
		TypeCPowerWell well(Ops(m), 2, 2); Combined c = {&well, false};
		TypeCPowerOps ops = {&c, AcquireCombined, ReleaseCombined, OwnedCombined,
			SetCombined, ReadyCombined};
		TypeCPowerLifecycle lifecycle(ops);
		if (lifecycle.Acquire() != B_OK || !c.owned || !well.IsHeld()
			|| lifecycle.Release() != B_OK || c.owned || well.IsHeld()) errors++;
	}
	{
		MockWell m = {0, false, false, false, true, 0, 0};
		TypeCPowerWell well(Ops(m), 2, 2); Combined c = {&well, false};
		TypeCPowerOps ops = {&c, AcquireCombined, ReleaseCombined, OwnedCombined,
			SetCombined, ReadyCombined};
		TypeCPowerLifecycle lifecycle(ops);
		if (lifecycle.Acquire() == B_OK || !well.IsHeld()
			|| lifecycle.Acquire() != B_BUSY) errors++;
		m.failClear = false;
		if (lifecycle.Release() != B_OK || well.IsHeld()) errors++;
	}
	if (errors) { fprintf(stderr, "%u well cases failed\n", errors); return 1; }
	puts("14 Type-C power-well and lifecycle integration cases passed");
	return 0;
}
