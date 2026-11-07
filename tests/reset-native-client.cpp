// SPDX-License-Identifier: MIT
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "reset-native-protocol.h"
int main()
{
	alarm(10); // Userland only; cannot interrupt a stuck kernel MMIO.
	int fd = open("/dev/misc/intel_bcs_reset", O_RDONLY);
	if (fd < 0) { perror("open"); return 1; }
	BcsResetResult result = {};
	int status = ioctl(fd, BCS_RESET_IOCTL, &result, sizeof(result));
	if (status < 0) perror("reset");
	close(fd);
	if (status < 0) return 1;
	const auto& b = result.bootstrap;
	const auto& g = result.group;
	const auto& r = result.reset;
	const auto& s = result.safety;
	printf("version=%u module=%d transaction=%d fuse=%#x\n", result.version,
		int(result.module), int(result.transaction), result.rawFuse);
	printf("bootstrap error=%u cleanup=%u stage=%u ownership=%u ACK=%#x/%#x/%#x reads=%u writes=%u elapsed_us=%" PRIu64 "\n",
		b.error, b.cleanup, b.stage, b.ownership, b.initialAck, b.setAck, b.finalAck, b.reads, b.writes, b.elapsed);
	printf("group error=%u stage=%u domain=%u count=%u workCalled=%u elapsed_us=%" PRIu64 "\n",
		g.error, g.stage, g.domain, g.count, g.workCalled, g.elapsed);
	printf("reset error=%u stage=%u reads=%u writes=%u rounds=%u elapsed_us=%" PRIu64 " last=%#x/%#x attempted=%#x/%#x\n",
		r.error, r.stage, r.reads, r.writes, r.resetRounds, r.elapsed,
		r.lastOffset, r.lastValue, r.attemptedOffset, r.attemptedValue);
	printf("safety module=%d read=%d reason=%u checks=%u phase=%u domain=%u ackReads=%u pciResult=%u groupStage=%u groupCount=%u\n",
		int(result.safetyModule), int(result.safetyRead), s.reason, s.checks, s.phase,
		s.domain, s.ackReads, s.pciResult, s.groupStage, s.groupCount);
	printf("safety clock=%" PRIu64 " previous=%" PRIu64 " deadline=%" PRIu64 " ACK=%#x/%#x/%#x/%#x/%#x\n",
		s.now, s.previous, s.deadline, s.ack[0], s.ack[1], s.ack[2], s.ack[3], s.ack[4]);
	printf("safety PCI expected=%#x/%#x command=%#x BAR=%#x:%#x PM=%#x; observed=%#x/%#x command=%#x BAR=%#x:%#x PM=%#x\n",
		s.expected.identity, s.expected.classRevision, s.expected.command,
		s.expected.barHigh, s.expected.barLow, s.expected.pmControl,
		s.observed.identity, s.observed.classRevision, s.observed.command,
		s.observed.barHigh, s.observed.barLow, s.observed.pmControl);
	IntelN100ResetDomains plan = {};
	bool valid = result.version == 2 && result.module == B_OK && result.transaction == B_OK
		&& result.safetyModule == B_OK && result.safetyRead == B_OK
		&& s.reason == IntelN100ResetSafety::None && s.checks > 0
		&& b.error == 0 && b.cleanup == 0 && b.stage == IntelN100Forcewake::Complete
		&& b.ownership == IntelN100Forcewake::Released && b.writes == 2 && b.reads >= 5
		&& (b.initialAck & 0x8001) == 0 && (b.setAck & 0x8001) == 1
		&& (b.finalAck & 0x8001) == 0 && b.elapsed < 1000000
		&& intel_n100_reset_domains(0x46d18086, 0, result.rawFuse, plan)
		&& g.error == 0 && g.stage == IntelN100ForcewakeGroup::Complete
		&& g.count == plan.count && g.workCalled == 1 && g.elapsed < 1000000
		&& r.error == 0 && r.stage == IntelN100BcsReset::ResetObserved
		&& r.writes == 6 && r.resetRounds == 2 && r.elapsed < 10000
		&& r.lastOffset == IntelN100BcsReset::kResetControl && (r.lastValue & 5) == 0
		&& r.attemptedOffset == IntelN100BcsReset::kResetControl && r.attemptedValue == 0x10000;
	for (unsigned i = 0; i < g.count && i < 5; i++) {
		const auto& d = g.domains[i];
		printf("domain=%u request=%#x ack=%#x ownership=%u ACK=%#x/%#x/%#x last=%#x reads=%u writes=%u\n",
			d.kind, d.request, d.ack, d.ownership, d.initialAck, d.setAck, d.finalAck, d.lastAck, d.reads, d.writes);
		valid = valid && i < plan.count && d.kind == plan.domains[i].kind
			&& d.request == plan.domains[i].request && d.ack == plan.domains[i].ack
			&& d.ownership == IntelN100ForcewakeGroup::Released && d.writes == 2 && d.reads >= 5
			&& (d.initialAck & 0x8001) == 0 && (d.setAck & 0x8001) == 1
			&& (d.finalAck & 0x8001) == 0 && (d.lastAck & 0x8001) == 0;
	}
	puts("Single-use BCS reset observation only; no engine initialization or GPU command execution.");
	return valid ? 0 : 1;
}
