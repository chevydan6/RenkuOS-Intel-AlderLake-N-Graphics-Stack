// SPDX-License-Identifier: MIT
#include "BcsEngine.h"
#include <assert.h>
#include <stdio.h>
#include <vector>
struct Write { uint32_t offset, value; };
struct Fixture {
	uint64_t time = 0;
	uint32_t hws = 0;
	bool visible = false, failLoad = false;
	std::vector<Write> writes;
	static uint64_t Now(void* p) { return ++((Fixture*)p)->time; }
	static bool Safe(void*) { return true; }
	static bool Visible(void* p) { ((Fixture*)p)->visible = true; return true; }
	static bool Read(void* p, uint32_t offset, uint32_t& value) {
		auto& f = *(Fixture*)p;
		value = offset == 0x22080 ? f.hws : offset == 0x2209c ? 0x200
			: offset == 0x223a0 ? 0xb0b : 0;
		return true;
	}
	static bool Store(void* p, uint32_t offset, uint32_t value) {
		auto& f = *(Fixture*)p;
		f.writes.push_back({offset, value});
		if (offset == 0x22080) f.hws = value;
		if (offset == 0x22550) { assert(f.visible); return !f.failLoad; }
		return true;
	}
	NativeGpu::BcsEngine::IO Io() { return {this, Now, Safe, Read, Store, Visible}; }
};
int main()
{
	uint64_t descriptor;
	assert(intel_n100_bcs_descriptor(0x46d18086, 0, 0x40000, 1, &descriptor));
	Fixture f;
	NativeGpu::BcsEngine engine;
	assert(engine.Initialize(f.Io(), 0x46d18086, 0, 0x20000, 10000));
	assert(!engine.LastReport().loadAttempted);
	assert(engine.Publish(f.Io(), descriptor));
	const Write expected[] = {{0x22518, 0}, {0x2251c, 0},
		{0x22510, uint32_t(descriptor)}, {0x22514, uint32_t(descriptor >> 32)},
		{0x22550, 1}};
	assert(f.writes.size() >= 5);
	for (unsigned i = 0; i < 5; i++) {
		const auto& actual = f.writes[f.writes.size() - 5 + i];
		assert(actual.offset == expected[i].offset && actual.value == expected[i].value);
	}
	assert(engine.LastReport().stage == NativeGpu::BcsEngine::Published);
	// A failed posting write may already have reached hardware. Keep the
	// publication ledger; the caller must retain all backing and wake ownership.
	Fixture failed; failed.failLoad = true;
	NativeGpu::BcsEngine uncertain;
	assert(uncertain.Initialize(failed.Io(), 0x46d18086, 0, 0x20000, 10000));
	assert(!uncertain.Publish(failed.Io(), descriptor));
	assert(uncertain.LastReport().loadAttempted);
	assert(uncertain.LastReport().error == NativeGpu::BcsEngine::WriteFailed);
	assert(!uncertain.Publish(failed.Io(), descriptor));
	puts("BCS init/ELSQ smoke: PASS (simulated IO, not GPU execution)");
}
