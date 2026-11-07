// SPDX-License-Identifier: MIT
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "group-native-protocol.h"
int main()
{
	alarm(10); // Userland only; cannot interrupt a stuck kernel MMIO.
	int fd = open("/dev/misc/intel_group_handshake", O_RDONLY);
	if (fd < 0) { perror("open"); return 1; }
	GroupHandshakeResult result = {};
	int status = ioctl(fd, GROUP_HANDSHAKE_IOCTL, &result, sizeof(result));
	if (status < 0) perror("handshake");
	close(fd);
	if (status < 0) return 1;
	const auto& b = result.bootstrap;
	const auto& g = result.group;
	printf("version=%u module=%d transaction=%d fuse=%#x\n", result.version,
		int(result.module), int(result.transaction), result.rawFuse);
	printf("bootstrap error=%u cleanup=%u stage=%u ownership=%u ACK=%#x/%#x/%#x reads=%u writes=%u elapsed_us=%" PRIu64 "\n",
		b.error, b.cleanup, b.stage, b.ownership, b.initialAck, b.setAck, b.finalAck, b.reads, b.writes, b.elapsed);
	printf("group error=%u stage=%u domain=%u count=%u workCalled=%u elapsed_us=%" PRIu64 "\n",
		g.error, g.stage, g.domain, g.count, g.workCalled, g.elapsed);
	IntelN100ResetDomains plan = {};
	bool valid = result.version == 1 && result.module == B_OK && result.transaction == B_OK
		&& b.error == 0 && b.cleanup == 0 && b.stage == IntelN100Forcewake::Complete
		&& b.ownership == IntelN100Forcewake::Released && b.writes == 2 && b.reads >= 5
		&& (b.initialAck & 0x8001) == 0 && (b.setAck & 0x8001) == 1
		&& (b.finalAck & 0x8001) == 0 && b.elapsed < 1000000
		&& intel_n100_reset_domains(0x46d18086, 0, result.rawFuse, plan)
		&& g.error == 0 && g.stage == IntelN100ForcewakeGroup::Complete
		&& g.count == plan.count && g.workCalled == 0 && g.elapsed < 1000000;
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
	puts("Single-use all-domain handshake only; no reset or GPU command execution.");
	return valid ? 0 : 1;
}
