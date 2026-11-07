// SPDX-License-Identifier: MIT
#include "IntelN100ForcewakeGroup.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <utility>
#include <vector>
using Group = IntelN100ForcewakeGroup;

struct Mock {
	uint64_t clock = 100, tick = 1;
	uint32_t ack[5] = {};
	unsigned reads = 0, writes = 0, checks = 0, times = 0, work = 0;
	int failRead = -1, onesRead = -1, fallbackRead = -1, failWrite = -1;
	int unsafe = -1, reverse = -1, jump = -1, noSet = -1, noClear = -1;
	int loseRead = -1, staleRead = -1;
	bool workFails = false, workOverruns = false, workLosesAck = false;
	Group* group = nullptr;
	std::vector<std::pair<uint32_t,uint32_t>> trace;
	static int Domain(uint32_t offset, bool request)
	{
		const uint32_t requests[] = {0xa188,0xa278,0xa540,0xa548,0xa560};
		const uint32_t acks[] = {0x130044,0xd84,0xd50,0xd58,0xd70};
		for (int i = 0; i < 5; i++) if (offset == (request ? requests[i] : acks[i])) return i;
		assert(false); return -1;
	}
	static uint64_t Now(void* p)
	{
		auto& m = *(Mock*)p; int n = m.times++;
		if (n == m.reverse) return m.clock - 20;
		if (n == m.jump) m.clock += 1000000;
		m.clock += m.tick; return m.clock;
	}
	static bool Safe(void* p) { auto& m = *(Mock*)p; return int(m.checks++) != m.unsafe; }
	static bool Read(void* p, uint32_t offset, uint32_t& value)
	{
		auto& m = *(Mock*)p; int n = m.reads++;
		if (n == m.failRead) return false;
		value = m.ack[Domain(offset, false)];
		if (n == m.onesRead) value = UINT32_MAX;
		if (n == m.fallbackRead) value |= 0x8000;
		if (n == m.loseRead) value = 0;
		if (n == m.staleRead) value |= 1;
		return true;
	}
	static bool Write(void* p, uint32_t offset, uint32_t value)
	{
		auto& m = *(Mock*)p; int n = m.writes++, d = Domain(offset, true);
		assert(value == 0x10001 || value == 0x10000);
		const auto& report = m.group->LastReport();
		assert(report.domains[report.domain].request == offset);
		assert(report.domains[report.domain].ownership ==
			(value == 0x10001 ? Group::EnableAttempted : Group::ClearAttempted));
		m.trace.emplace_back(offset,value);
		if (value == 0x10001 && d != m.noSet) m.ack[d] = 1;
		if (value == 0x10000 && d != m.noClear) m.ack[d] = 0;
		return n != m.failWrite; // Ambiguous failure after the actual store.
	}
	static void Pause(void*) {}
	static bool Work(void* p, uint64_t deadline)
	{
		auto& m = *(Mock*)p; m.work++;
		const auto& r = m.group->LastReport();
		for (unsigned i = 0; i < r.count; i++) {
			assert(r.domains[i].ownership == Group::Held);
			assert(m.ack[r.domains[i].kind] == 1);
		}
		assert(deadline > m.clock);
		if (m.workOverruns) m.clock = deadline;
		if (m.workLosesAck) m.ack[0] = 0;
		return !m.workFails;
	}
	Group::IO IO() { return {this,Now,Safe,Read,Write,Pause}; }
};

static unsigned cases;
static Group::Report Run(Mock& m, Group::Error expected, uint32_t fuse = 0,
	bool operation = false)
{
	Group group; m.group = &group;
	assert(group.Run(m.IO(),0x46d18086,0,fuse,operation ? Mock::Work : nullptr) == expected);
	auto report = group.LastReport(); assert(report.error == expected);
	unsigned reads = 0, writes = 0;
	for (unsigned i = 0; i < report.count; i++) { reads += report.domains[i].reads; writes += report.domains[i].writes; }
	assert(reads == m.reads && writes == m.writes && reads < 1000 && writes <= 10);
	if (expected == Group::Ok) {
		assert(report.stage == Group::Complete);
		for (unsigned i = 0; i < report.count; i++) {
			const auto& d = report.domains[i];
			assert(d.ownership == Group::Released && d.writes == 2);
			assert(d.initialAck == 0 && d.setAck == 1 && d.finalAck == 0);
		}
	}
	unsigned times = m.times, checks = m.checks;
	assert(group.Run(m.IO(),0x46d18086,0,fuse) == Group::AlreadyUsed);
	assert(memcmp(&report,&group.LastReport(),sizeof(report)) == 0);
	assert(m.reads == reads && m.writes == writes && m.times == times && m.checks == checks);
	m.group = nullptr; cases++; return report;
}

int main()
{
	Mock good; auto baseline = Run(good,Group::Ok,0,true);
	assert(good.work == 1 && baseline.count == 5);
	const uint32_t requests[] = {0xa188,0xa278,0xa540,0xa548,0xa560};
	for (unsigned i = 0; i < 5; i++) {
		assert(good.trace[i].first == requests[i] && good.trace[i].second == 0x10001);
		assert(good.trace[5+i].first == requests[4-i] && good.trace[5+i].second == 0x10000);
	}
	for (unsigned mask = 0; mask < 8; mask++) {
		uint32_t fuse = (mask & 1) | ((mask & 2) << 1) | ((mask & 4) << 14);
		Mock m; auto report = Run(m,Group::Ok,fuse,true);
		assert(report.count == 5 - unsigned(__builtin_popcount(mask)));
	}
	{ Mock m; auto r = Run(m,Group::Ok,0xe00fe); assert(r.count == 4 && m.writes == 8); }
	for (unsigned i = 0; i < good.reads; i++) {
		Mock m; m.failRead = i; Run(m,Group::ReadError,0,true); assert(m.reads == i+1);
		Mock ones; ones.onesRead = i; Run(ones,Group::ReadError,0,true); assert(ones.reads == i+1);
		Mock fallback; fallback.fallbackRead = i; Run(fallback,Group::FallbackBusy,0,true); assert(fallback.reads == i+1);
	}
	for (unsigned i = 0; i < good.writes; i++) {
		Mock m; m.failWrite = i; auto r = Run(m,Group::WriteError,0,true);
		assert(m.writes == i+1 && r.stage != Group::Complete);
		assert((m.trace == std::vector<std::pair<uint32_t,uint32_t>>(good.trace.begin(),good.trace.begin()+i+1)));
	}
	for (unsigned i = 0; i < good.checks; i++) {
		Mock m; m.unsafe = i; Run(m,Group::Unsafe,0,true); assert(m.checks == i+1);
	}
	for (unsigned i = 1; i < good.times; i++) {
		Mock m; m.reverse = i; Run(m,Group::ClockError,0,true); assert(m.times == i+1);
		Mock jump; jump.jump = i; Run(jump,Group::TotalTimeout,0,true); assert(jump.times == i+1);
	}
	for (unsigned i = 0; i < 5; i++) {
		Mock stale; stale.ack[i] = 1; Run(stale,Group::PreexistingRequest); assert(stale.writes == 0);
		Mock set; set.noSet = i; Run(set,Group::AckTimeout); assert(set.writes == i+1);
		Mock clear; clear.noClear = i; Run(clear,Group::AckTimeout); assert(clear.writes == 10-i);
		Mock frozen; frozen.tick = 0; frozen.noSet = i; Run(frozen,Group::AckTimeout);
		Mock elapsed; elapsed.tick = 100; elapsed.noClear = i; Run(elapsed,Group::AckTimeout);
	}
	{ Mock m; m.staleRead = 5; Run(m,Group::PreexistingRequest); assert(m.writes == 0); }
	{ Mock m; m.loseRead = 7; Run(m,Group::AckLost); assert(m.writes == 1); }
	{ Mock m; m.workFails = true; Run(m,Group::WorkFailed,0,true); assert(m.writes == 5); }
	{ Mock m; m.workOverruns = true; Run(m,Group::WorkTimeout,0,true); assert(m.writes == 5); }
	{ Mock m; m.workLosesAck = true; Run(m,Group::AckLost,0,true); assert(m.writes == 5); }
	{ Mock m; m.clock = UINT64_MAX-100; Run(m,Group::ClockError); assert(m.reads == 0); }
	for (unsigned i = 0; i < 8; i++) {
		Mock m; Group g; auto io = m.IO(); uint32_t id = 0x46d18086, fuse = 0; uint8_t rev = 0;
		if (i == 0) io.now = nullptr;
		if (i == 1) io.safe = nullptr;
		if (i == 2) io.readAck = nullptr;
		if (i == 3) io.writeRequest = nullptr;
		if (i == 4) io.pause = nullptr;
		if (i == 5) id++;
		if (i == 6) rev++;
		if (i == 7) fuse = UINT32_MAX;
		assert(g.Run(io,id,rev,fuse) == (i < 5 ? Group::InvalidIO : Group::InvalidPlan));
		assert(m.reads == 0 && m.writes == 0 && m.times == 0); cases++;
	}
	printf("%u multi-domain forcewake cases passed; mocked I/O only.\n",cases);
}
