// SPDX-License-Identifier: MIT
#include "IntelN100ContextStatus.h"
#include <assert.h>
#include <initializer_list>
#include <stdio.h>
#include <string.h>

using Status = IntelN100ContextStatus;
// Test vectors use the documented bit positions, independent of the decoder.
static uint64_t Event(uint32_t to, uint32_t away, unsigned detail = 0, bool queue = false)
{
	return (uint64_t(away) << 47) | (uint64_t(detail) << 32)
		| (uint64_t(to) << 15) | 0x18 | uint64_t(queue);
}
static void CheckFailure(Status& s, uint64_t generation, uint64_t raw, Status::Error error)
{
	assert(s.Observe(generation, raw) == Status::Rejected);
	assert(s.LastReport().state == Status::Uncertain && s.LastReport().error == error);
	Status::Report saved = s.LastReport();
	assert(!s.Arm(generation + 1, 7));
	assert(s.Observe(generation, Event(0x7ff, 7)) == Status::Rejected);
	assert(memcmp(&saved, &s.LastReport(), sizeof(saved)) == 0);
}
int main()
{
	unsigned cases = 0;
	uint64_t descriptor = 0;
	assert(intel_n100_bcs_descriptor(0x46d18086, 0, 0x1234000, 1, &descriptor));
	assert(descriptor == 0x600000200123411dULL); cases++;
	for (uint32_t tag = 1; tag < 0x7ff; tag++) {
		assert(intel_n100_bcs_descriptor(0x46d18086, 0, 0xffffc000, tag, &descriptor));
		assert((descriptor & 0xffffffffULL) == 0xffffc11d);
		assert(((descriptor >> 37) & 0x7ff) == tag);
		assert((descriptor >> 61) == 3 && ((descriptor >> 48) & 0x1fff) == 0);
		Status s;
		assert(s.Arm(42, tag));
		assert(s.Observe(42, UINT64_MAX) == Status::AwaitingEntryWrite);
		assert(s.LastReport().state == Status::AwaitingActivation && s.LastReport().accepted == 0);
		assert(s.Observe(42, Event(tag, 0x7ff, 0, true)) == Status::Accepted);
		assert(s.LastReport().state == Status::Active);
		assert(s.Observe(42, UINT64_MAX) == Status::AwaitingEntryWrite);
		assert(s.LastReport().state == Status::Active && s.LastReport().accepted == 1);
		assert(s.Observe(42, Event(0x7ff, tag)) == Status::Accepted);
		assert(s.LastReport().state == Status::SwitchedAway && s.LastReport().accepted == 2);
		assert(s.LastReport().raw[0] == Event(tag, 0x7ff, 0, true));
		assert(s.LastReport().raw[1] == Event(0x7ff, tag));
		assert(!s.Arm(43, tag)); cases++;
	}
	const uint64_t badAddresses[] = {1, 4095, 4097, 0xffffd000, 0xfffff000, 1ULL << 32, UINT64_MAX};
	for (uint64_t address : badAddresses) {
		descriptor = 0xabcdef;
		assert(!intel_n100_bcs_descriptor(0x46d18086, 0, address, 7, &descriptor));
		assert(descriptor == 0xabcdef); cases++;
	}
	for (uint32_t tag : {0u, 0x7ffu, 0x800u, UINT32_MAX}) {
		descriptor = 0xabcdef;
		assert(!intel_n100_bcs_descriptor(0x46d18086, 0, 4096, tag, &descriptor));
		assert(descriptor == 0xabcdef);
		Status s; assert(!s.Arm(1, tag) && s.LastReport().state == Status::Empty); cases++;
	}
	assert(!intel_n100_bcs_descriptor(0x46d18086, 0, 0, 1, nullptr)); cases++;
	for (uint32_t identity : {0u, 0x46d08086u, 0x46d28086u, UINT32_MAX}) {
		descriptor = 0xabcdef;
		assert(!intel_n100_bcs_descriptor(identity, 0, 4096, 1, &descriptor));
		assert(descriptor == 0xabcdef); cases++;
	}
	for (unsigned revision = 1; revision < 256; revision++) {
		descriptor = 0xabcdef;
		assert(!intel_n100_bcs_descriptor(0x46d18086, revision, 4096, 1, &descriptor));
		assert(descriptor == 0xabcdef); cases++;
	}
	Status empty; assert(!empty.Arm(0, 7));
	CheckFailure(empty, 1, Event(7, 0x7ff), Status::NotArmed); cases++;
	for (unsigned active = 0; active < 2; active++) {
		for (unsigned fault = 0; fault < 10; fault++) {
			Status s; assert(s.Arm(42, 7));
			if (active) assert(s.Observe(42, Event(7, 0x7ff)) == Status::Accepted);
			uint64_t raw = active ? Event(0x7ff, 7) : Event(7, 0x7ff);
			uint64_t generation = 42;
			Status::Error expected = Status::UnexpectedTransition;
			switch (fault) {
				case 0: generation = 41; expected = Status::WrongGeneration; break;
				case 1: raw ^= 8; expected = Status::WrongEngine; break;
				case 2: raw |= 64; expected = Status::WrongEngine; break;
				case 3: raw = active ? Event(0x7ff, 8) : Event(8, 0x7ff); expected = Status::WrongTag; break;
				case 4: raw |= active ? (1ULL << 58) : (1ULL << 26); expected = Status::WrongCounter; break;
				case 5: raw = Event(0x7ff, 0x7ff); break;
				case 6: raw = Event(7, 7); break;
				case 7: raw = active ? Event(7, 0x7ff) : Event(0x7ff, 7); break;
				case 8: raw = active ? Event(0x7ff, 7, 0, true) : Event(7, 7, 0, true); break;
				case 9: raw = Event(0x7ff, 7, 5); break;
			}
			CheckFailure(s, generation, raw, expected); cases++;
		}
	}
	// Every nonzero switch detail is unsupported after activation, including
	// wait-based preemption which must not be inferred from detail==5 alone.
	for (unsigned detail = 1; detail < 16; detail++) {
		Status s; assert(s.Arm(42, 7));
		assert(s.Observe(42, Event(7, 0x7ff)) == Status::Accepted);
		CheckFailure(s, 42, Event(0x7ff, 7, detail), Status::UnexpectedTransition); cases++;
	}
	// Ignored fields are retained, not interpreted as tag/retirement evidence.
	Status ignored; assert(ignored.Arm(42, 7));
	uint64_t in = Event(7, 0x7ff, 15) | (63ULL << 58) | (127ULL << 37) | 4;
	uint64_t out = Event(0x7ff, 7) | (63ULL << 26) | (127ULL << 37) | 4;
	assert(ignored.Observe(42, in) == Status::Accepted);
	assert(ignored.Observe(42, out) == Status::Accepted);
	assert(ignored.LastReport().raw[0] == in && ignored.LastReport().raw[1] == out); cases++;
	CheckFailure(ignored, 42, out, Status::UnexpectedTransition); cases++;
	uint64_t vector = (51ULL << 58) | (1234ULL << 47) | (93ULL << 37)
		| (5ULL << 32) | (37ULL << 26) | (567ULL << 15) | (42ULL << 6) | (3ULL << 3) | 5;
	auto e = intel_n100_csb_decode(vector);
	assert(e.raw == vector && e.toTag == 567 && e.awayTag == 1234
		&& e.toCounter == 37 && e.awayCounter == 51 && e.engineClass == 3
		&& e.instance == 42 && e.detail == 5 && e.waitDetail == 93 && e.newQueue && e.semaphoreMode); cases++;
	printf("%u context descriptor/status cases passed; offline software only, no retirement authority.\n", cases);
}
