// SPDX-License-Identifier: MIT
#include "IntelN100CsbReader.h"
#include <assert.h>
#include <initializer_list>
#include <stdio.h>
#include <vector>

using Reader = IntelN100CsbReader;
using Status = IntelN100ContextStatus;
static constexpr uint64_t kIn = (0x7ffULL << 47) | (7ULL << 15) | 0x19;
static constexpr uint64_t kOut = (7ULL << 47) | (0x7ffULL << 15) | 0x18;
struct Fake {
	uint64_t time = 100, step = 1, entries[2] = {kIn, kOut};
	uint32_t tail = 1, nextTail = 1;
	unsigned ticks = 0, nowCalls = 0, safeCalls = 0, tails = 0, reads = 0, barriers = 0;
	unsigned failNow = 0, failSafe = 0, failTail = 0, failEntry = 0, failBarrier = 0;
	uint64_t badTime = UINT64_MAX;
	bool delay = false, delayedSecond = false, changeTail = false, barrierSeen = false;
	std::vector<unsigned> slots;
	static uint64_t Now(void* p) {
		auto& f = *(Fake*)p;
		if (++f.nowCalls == f.failNow) return f.badTime;
		uint64_t value = f.time; f.time += f.step; return value;
	}
	static bool Safe(void* p) { auto& f = *(Fake*)p; return ++f.safeCalls != f.failSafe; }
	static bool Tail(void* p, uint32_t offset, uint32_t& value) {
		auto& f = *(Fake*)p; assert(offset == 0xbc);
		f.barrierSeen = false;
		if (++f.tails == f.failTail) return false;
		value = f.changeTail && f.tails > 1 ? f.nextTail : f.tail;
		return true;
	}
	static bool Entry(void* p, uint32_t offset, uint64_t& value) {
		auto& f = *(Fake*)p;
		assert(f.barrierSeen && (offset == 0x40 || offset == 0x48));
		unsigned slot = (offset - 0x40) / 8;
		f.slots.push_back(slot);
		if (++f.reads == f.failEntry) return false;
		value = f.entries[slot]; return true;
	}
	static bool Barrier(void* p) {
		auto& f = *(Fake*)p; f.barrierSeen = true;
		return ++f.barriers != f.failBarrier;
	}
	static void Relax(void* p) {
		auto& f = *(Fake*)p; ++f.ticks;
		if (f.delay) {
			if (f.ticks == 1) { f.tail = 0; f.entries[0] = UINT64_MAX; }
			if (f.ticks == 2) f.entries[0] = kIn;
			if (f.ticks == 3) { f.tail = 1; f.entries[1] = UINT64_MAX; }
			if (f.ticks == 4) f.entries[1] = kOut;
		}
		if (f.delayedSecond && f.ticks == 2) f.entries[1] = kOut;
	}
	Reader::Io Io() { return {this, Now, Safe, Tail, Entry, Barrier, Relax}; }
};
static void Retained(Reader& reader, Fake& fake)
{
	Reader::Report saved = reader.LastReport();
	unsigned calls = fake.nowCalls + fake.safeCalls + fake.tails + fake.reads + fake.barriers + fake.ticks;
	assert(!reader.Run(fake.Io(), 43, 8, 1000, 100));
	assert(memcmp(&saved, &reader.LastReport(), sizeof(saved)) == 0);
	assert(calls == fake.nowCalls + fake.safeCalls + fake.tails + fake.reads + fake.barriers + fake.ticks);
}
int main()
{
	unsigned cases = 0;
	alignas(4096) uint8_t storage[4096 + 16];
	memset(storage, 0x5a, sizeof(storage));
	assert(intel_n100_initialize_hwsp(storage + 8, 4096));
	for (unsigned i = 0; i < sizeof(storage); i++) {
		uint8_t expected = 0;
		if (i < 8 || i >= 4104) expected = 0x5a;
		else if (i - 8 >= 0x40 && i - 8 < 0xa0) expected = 0xff;
		else if (i - 8 == 0xbc) expected = 11;
		assert(storage[i] == expected);
	}
	cases++;
	uint8_t saved[sizeof(storage)]; memcpy(saved, storage, sizeof(saved));
	for (size_t size : {size_t(0), size_t(4095), size_t(4097), SIZE_MAX}) {
		assert(!intel_n100_initialize_hwsp(storage + 8, size));
		assert(memcmp(storage, saved, sizeof(saved)) == 0); cases++;
	}
	for (unsigned offset = 1; offset < 8; offset++) {
		assert(!intel_n100_initialize_hwsp(storage + offset, 4096));
		assert(memcmp(storage, saved, sizeof(saved)) == 0); cases++;
	}
	assert(!intel_n100_initialize_hwsp(nullptr, 4096)); cases++;
	assert(!intel_n100_initialize_hwsp(reinterpret_cast<void*>(UINTPTR_MAX - 7), 4096)); cases++;

	Fake baseline; Reader pass;
	assert(pass.Run(baseline.Io(), 42, 7, 1000, 100));
	assert(pass.LastReport().phase == Reader::EventsObserved && pass.LastReport().error == Reader::None);
	assert(pass.LastReport().events.state == Status::SwitchedAway && pass.LastReport().events.accepted == 2);
	assert(baseline.tails == 2 && baseline.reads == 2 && baseline.barriers == 1 && baseline.ticks == 0);
	assert(baseline.slots == std::vector<unsigned>({0, 1}));
	assert(pass.LastReport().elapsed < 1000);
	Retained(pass, baseline); cases++;
	Fake delayed; delayed.delay = true; delayed.tail = 11;
	Reader wait; assert(wait.Run(delayed.Io(), 42, 7, 1000, 100));
	assert(delayed.ticks == 4 && delayed.slots == std::vector<unsigned>({0, 0, 1, 1}));
	Retained(wait, delayed); cases++;
	Fake second; second.delayedSecond = true; second.entries[1] = UINT64_MAX;
	Reader pending; assert(pending.Run(second.Io(), 42, 7, 1000, 100));
	assert(second.slots == std::vector<unsigned>({0, 1, 1, 1})); cases++;

	// Every callback position in the successful path can fail, including final
	// tail/safety checks after both events. First failure is durable and terminal.
	for (unsigned kind = 0; kind < 7; kind++) {
		unsigned count = kind < 3 ? baseline.nowCalls : kind == 3 ? baseline.safeCalls
			: kind == 4 ? baseline.tails : kind == 5 ? baseline.reads : baseline.barriers;
		for (unsigned n = 1; n <= count; n++) {
			Fake f; Reader r; Reader::Error expected;
			if (kind < 3) {
				f.failNow = n; f.badTime = kind == 0 ? UINT64_MAX : kind == 1 ? 0 : 1100;
				// A different first timestamp just establishes a different origin.
				if (n == 1 && kind != 0) continue;
				expected = kind == 2 ? Reader::Deadline : Reader::Clock;
			} else if (kind == 3) { f.failSafe = n; expected = Reader::Unsafe; }
			else if (kind == 4) { f.failTail = n; expected = Reader::TailRead; }
			else if (kind == 5) { f.failEntry = n; expected = Reader::EntryRead; }
			else { f.failBarrier = n; expected = Reader::Visibility; }
			assert(!r.Run(f.Io(), 42, 7, 1000, 100));
			assert(r.LastReport().phase == Reader::Failed && r.LastReport().error == expected);
			Retained(r, f); cases++;
		}
	}
	for (uint32_t badTail : {2u, 3u, 10u, 12u, 15u, 16u, 0x100u, UINT32_MAX}) {
		for (unsigned final = 0; final < 2; final++) {
			Fake f; f.tail = final ? 1 : badTail; f.changeTail = final; f.nextTail = badTail;
			Reader r; assert(!r.Run(f.Io(), 42, 7, 1000, 100));
			assert(r.LastReport().error == Reader::UnexpectedTail);
			assert(f.reads == (final ? 2u : 0u)); cases++;
		}
	}
	for (uint32_t tail : {0u, 11u}) {
		for (unsigned poison = 0; poison < 2; poison++) {
			Fake f; f.changeTail = true; f.nextTail = tail;
			if (poison) f.entries[0] = UINT64_MAX;
			Reader r; assert(!r.Run(f.Io(), 42, 7, 1000, 100));
			assert(r.LastReport().error == Reader::TailRegression); cases++;
		}
	}
	for (unsigned variant = 0; variant < 3; variant++) {
		Fake f; f.step = 0;
		if (variant == 0) f.tail = 11;
		if (variant == 1) f.entries[0] = UINT64_MAX;
		if (variant == 2) f.entries[1] = UINT64_MAX;
		Reader r; assert(!r.Run(f.Io(), 42, 7, 250000, 7));
		assert(r.LastReport().error == Reader::PollLimit && r.LastReport().polls == 7);
		assert(r.LastReport().events.accepted == (variant == 2 ? 1u : 0u));
		if (variant == 1) for (unsigned slot : f.slots) assert(slot == 0);
		if (variant == 2) assert(f.slots == std::vector<unsigned>({0, 1, 1, 1, 1, 1, 1, 1}));
		Retained(r, f); cases++;
	}
	for (unsigned fault = 0; fault < 4; fault++) {
		Fake f;
		if (fault == 0) f.entries[0] ^= 1ULL << 15; // Foreign activation tag.
		if (fault == 1) f.entries[1] |= 5ULL << 32; // Preemption, not completion.
		if (fault == 2) f.entries[0] = kOut; // Completion before activation.
		if (fault == 3) f.entries[1] = kIn; // Duplicate activation.
		Reader r; assert(!r.Run(f.Io(), 42, 7, 1000, 100));
		assert(r.LastReport().error == Reader::EventRejected && r.LastReport().events.state == Status::Uncertain);
		Retained(r, f); cases++;
	}
	for (unsigned invalid = 0; invalid < 14; invalid++) {
		Fake f; auto io = f.Io(); Reader r;
		uint64_t generation = 42, budget = 1000; uint32_t tag = 7, limit = 100;
		switch (invalid) {
			case 0: io.now = nullptr; break;
			case 1: io.safe = nullptr; break;
			case 2: io.read32 = nullptr; break;
			case 3: io.read64 = nullptr; break;
			case 4: io.acquire = nullptr; break;
			case 5: io.relax = nullptr; break;
			case 6: generation = 0; break;
			case 7: tag = 0; break;
			case 8: tag = 0x7ff; break;
			case 9: budget = 0; break;
			case 10: budget = 250001; break;
			case 11: limit = 0; break;
			case 12: limit = 65537; break;
			case 13: tag = UINT32_MAX; break;
		}
		assert(!r.Run(io, generation, tag, budget, limit));
		assert(r.LastReport().error == Reader::InvalidInput && f.nowCalls == 0 && f.safeCalls == 0 && f.reads == 0 && f.tails == 0);
		Retained(r, f); cases++;
	}
	Fake overflow; overflow.time = UINT64_MAX - 999;
	Reader r; assert(!r.Run(overflow.Io(), 42, 7, 1000, 100));
	assert(r.LastReport().error == Reader::Clock && overflow.tails == 0); cases++;
	puts("Injected callbacks only; no GPU access, completion or retirement authority.");
	printf("%u bounded CSB reader/HWSP cases passed.\n", cases);
}
