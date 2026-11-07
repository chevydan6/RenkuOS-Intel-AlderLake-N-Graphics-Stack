// SPDX-License-Identifier: MIT
#include "IntelN100BcsReset.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <utility>
#include <vector>

using Reset = IntelN100BcsReset;
struct Hardware {
	uint64_t clock = 100, step = 1;
	unsigned reads = 0, writes = 0, checks = 0, times = 0, pauses = 0;
	int failRead = -1, onesRead = -1, failWrite = -1, unsafe = -1;
	int reverseTime = -1, jumpTime = -1;
	uint32_t control = 0, mode = 0x200, prefetch = 0, reset = 0, domain = 0;
	uint32_t pending = 0;
	bool stopTimeout = false, readyTimeout = false, cancelTimeout = false;
	bool catastrophe = false, foreignDomain = false;
	unsigned domainTimeout = 0, rounds = 0;
	bool frozenSettle = false, slowRead = false, slowWrite = false;
	Reset* owner = nullptr;
	std::vector<std::pair<uint32_t, uint32_t>> trace;
	static uint64_t Now(void* data)
	{
		auto& h = *(Hardware*)data;
		unsigned n = h.times++;
		if (int(n) == h.reverseTime) return h.clock - 20;
		if (int(n) == h.jumpTime) h.clock += 10000;
		h.clock += h.step;
		return h.clock;
	}
	static bool Safe(void* data)
	{
		auto& h = *(Hardware*)data;
		return int(h.checks++) != h.unsafe;
	}
	static bool Read(void* data, uint32_t offset, uint32_t& value)
	{
		auto& h = *(Hardware*)data;
		int n = h.reads++;
		if (n == h.failRead) return false;
		switch (offset) {
			case 0x2203c: value = h.control; break;
			case 0x2209c: value = h.mode; break;
			case 0x2229c: value = h.prefetch; break;
			case 0x220d0: value = h.reset; break;
			case 0x800c: value = h.pending; break;
			case 0x941c:
				if (h.rounds != 0 && h.domainTimeout != h.rounds) h.domain = 0;
				value = h.domain;
				if (h.foreignDomain && h.rounds != 0) value |= 8;
				if (h.frozenSettle && h.rounds == 2) h.step = 0;
				break;
			default: assert(false);
		}
		if (n == h.onesRead) value = UINT32_MAX;
		if (h.slowRead) h.clock += 10000;
		return true;
	}
	static bool Write(void* data, uint32_t offset, uint32_t value)
	{
		auto& h = *(Hardware*)data;
		// Verify attempted-store ownership is persisted BEFORE invoking I/O.
		assert(h.owner->LastReport().writes == h.writes + 1);
		assert(h.owner->LastReport().attemptedOffset == offset);
		assert(h.owner->LastReport().attemptedValue == value);
		h.trace.emplace_back(offset, value);
		int n = h.writes++;
		if (offset == 0x2209c && value == 0x01000100)
			h.mode = h.stopTimeout ? 0x100 : 0x300;
		else if (offset == 0x2229c && value == 0x04000400) h.prefetch |= 0x400;
		else if (offset == 0x220d0 && value == 0x00010001)
			h.reset = h.catastrophe ? 7 : h.readyTimeout ? 1 : 3;
		else if (offset == 0x941c && value == 4) { h.domain = 4; h.rounds++; }
		else if (offset == 0x220d0 && value == 0x00010000) {
			if (!h.cancelTimeout) h.reset &= ~1u;
		} else assert(false); // Never accept a generic or cross-engine store.
		if (h.slowWrite) h.clock += 10000;
		return n != h.failWrite; // Simulate a store that issued but reports failure.
	}
	static void Pause(void* data) { ((Hardware*)data)->pauses++; }
	Reset::IO IO() { return {this, Now, Safe, Read, Write, Pause}; }
};

static unsigned cases;
static Reset::Report Run(Hardware& hardware, Reset::Error expected)
{
	Reset reset;
	hardware.owner = &reset;
	assert(reset.Run(hardware.IO()) == expected);
	auto report = reset.LastReport();
	assert(report.error == expected);
	assert(report.reads == hardware.reads && report.writes == hardware.writes);
	assert(report.writes <= 6 && hardware.reads < 5000);
	assert(hardware.checks < 25000 && hardware.times < 60000);
	assert(reset.WasUsed());
	auto trace = hardware.trace;
	unsigned reads = hardware.reads, checks = hardware.checks, times = hardware.times;
	assert(reset.Run(hardware.IO()) == Reset::AlreadyUsed);
	assert(memcmp(&report, &reset.LastReport(), sizeof(report)) == 0);
	assert(hardware.trace == trace && hardware.reads == reads);
	assert(hardware.checks == checks && hardware.times == times);
	hardware.owner = nullptr;
	cases++;
	return report;
}

int main()
{
	Hardware good;
	auto report = Run(good, Reset::Ok);
	assert(report.stage == Reset::ResetObserved && report.resetRounds == 2);
	const std::vector<std::pair<uint32_t, uint32_t>> expected = {
		{0x2209c, 0x01000100}, {0x2229c, 0x04000400},
		{0x220d0, 0x00010001}, {0x941c, 4}, {0x941c, 4}, {0x220d0, 0x00010000}
	};
	assert(good.trace == expected);
	assert(good.pauses >= 16); // At least 50 us with 3 ticks/settling check.
	for (unsigned i = 0; i < good.reads; i++) {
		Hardware h; h.failRead = i;
		Run(h, Reset::ReadError); assert(h.reads == i + 1);
		Hardware ones; ones.onesRead = i;
		Run(ones, Reset::ReadError); assert(ones.reads == i + 1);
	}
	for (unsigned i = 0; i < good.writes; i++) {
		Hardware h; h.failWrite = i;
		auto r = Run(h, Reset::WriteError);
		assert(h.writes == i + 1 && r.stage != Reset::ResetObserved);
		assert((std::vector<std::pair<uint32_t, uint32_t>>(expected.begin(),
			expected.begin() + i + 1) == h.trace));
	}
	for (unsigned i = 0; i < good.checks; i++) {
		Hardware h; h.unsafe = i;
		Run(h, Reset::Unsafe); assert(h.checks == i + 1);
	}
	for (unsigned i = 1; i < good.times; i++) {
		Hardware h; h.reverseTime = i;
		Run(h, Reset::ClockError); assert(h.times == i + 1);
		Hardware jump; jump.jumpTime = i;
		Run(jump, Reset::Deadline); assert(jump.times == i + 1);
	}
	for (unsigned i = 0; i < 9; i++) {
		Hardware h;
		switch (i) {
			case 0: h.control = 1; break;
			case 1: h.mode = 0; break;
			case 2: h.mode = 0x300; break;
			case 3: h.prefetch = 0x400; break;
			case 4: h.reset = 1; break;
			case 5: h.reset = 2; break;
			case 6: h.reset = 4; break;
			case 7: h.domain = 4; break;
			case 8: h.domain = 8; break;
		}
		Run(h, Reset::PreexistingState); assert(h.writes == 0);
	}
	for (unsigned i = 0; i < 5; i++) {
		Hardware h;
		if (i == 0) h.stopTimeout = true;
		if (i == 1) h.readyTimeout = true;
		if (i == 2) h.domainTimeout = 1;
		if (i == 3) h.domainTimeout = 2;
		if (i == 4) h.cancelTimeout = true;
		auto r = Run(h, Reset::Timeout);
		const unsigned writes[] = {2, 3, 4, 5, 6};
		assert(r.writes == writes[i]);
		// Re-run each timeout scenario with a frozen clock: finite loop bound.
		h = Hardware(); h.step = 0;
		if (i == 0) h.stopTimeout = true;
		if (i == 1) h.readyTimeout = true;
		if (i == 2) h.domainTimeout = 1;
		if (i == 3) h.domainTimeout = 2;
		// With frozen time, success cannot pass the mandatory settling delay.
		if (i == 4) h.frozenSettle = true;
		Run(h, Reset::Timeout);
	}
	{ Hardware h; h.pending = 0x02000200; Run(h, Reset::PendingWake); assert(h.writes == 2); }
	// Both halves must indicate pending, not merely one of them.
	{ Hardware h; h.pending = 0x200; Run(h, Reset::Ok); }
	{ Hardware h; h.pending = 0x02000000; Run(h, Reset::Ok); }
	{ Hardware h; h.pending = 0x10; Run(h, Reset::Ok); } // Actual A29 sample.
	{ Hardware h; h.catastrophe = true; Run(h, Reset::PreexistingState); assert(h.rounds == 0); }
	{ Hardware h; h.foreignDomain = true; Run(h, Reset::PreexistingState); assert(h.rounds == 1); }
	{ Hardware h; h.frozenSettle = true; Run(h, Reset::Timeout); assert(h.writes == 5); }
	{ Hardware h; h.clock = UINT64_MAX - 10; Run(h, Reset::ClockError); assert(h.reads == 0); }
	{ Hardware h; h.slowRead = true; Run(h, Reset::Deadline); assert(h.writes == 0); }
	{ Hardware h; h.slowWrite = true; Run(h, Reset::Deadline); assert(h.writes == 1); }
	for (unsigned i = 0; i < 5; i++) {
		Hardware h; Reset reset; auto io = h.IO();
		if (i == 0) io.now = nullptr;
		if (i == 1) io.safe = nullptr;
		if (i == 2) io.read = nullptr;
		if (i == 3) io.write = nullptr;
		if (i == 4) io.pause = nullptr;
		assert(reset.Run(io) == Reset::InvalidIO);
		assert(h.reads == 0 && h.writes == 0 && h.times == 0);
		assert(reset.Run(h.IO()) == Reset::AlreadyUsed); cases++;
	}
	printf("%u BCS reset-only cases passed; mocked I/O, no hardware reset.\n", cases);
}
