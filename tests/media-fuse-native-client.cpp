// SPDX-License-Identifier: MIT
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "media-fuse-native-protocol.h"
#include "IntelN100ResetDomains.h"
int main()
{
	alarm(10); // Userland bound only; a stuck kernel MMIO cannot be interrupted.
	int fd = open("/dev/misc/intel_media_fuse", O_RDONLY);
	if (fd < 0) { perror("open"); return 1; }
	MediaFuseResult result = {};
	int status = ioctl(fd, MEDIA_FUSE_IOCTL, &result, sizeof(result));
	if (status < 0) perror("sample");
	close(fd);
	if (status < 0) return 1;
	const auto& r = result.report;
	printf("version=%u module=%d transaction=%d error=%u cleanup=%u stage=%u ownership=%u\n",
		result.version, int(result.module), int(result.transaction), r.error, r.cleanup, r.stage, r.ownership);
	printf("ACK=%#x/%#x/%#x reads=%u writes=%u elapsed_us=%" PRIu64 "\n",
		r.initialAck, r.setAck, r.finalAck, r.reads, r.writes, r.elapsed);
	bool valid = result.version == 1 && result.module == B_OK && result.transaction == B_OK
		&& r.error == 0 && r.cleanup == 0 && r.stage == IntelN100Forcewake::Complete
		&& r.ownership == IntelN100Forcewake::Released && r.writes == 2 && r.reads >= 5
		&& (r.initialAck & 0x8001) == 0 && (r.setAck & 0x8001) == 1
		&& (r.finalAck & 0x8001) == 0 && result.rawFuse != UINT32_MAX;
	if (valid) {
		IntelN100ResetDomains plan = {};
		// Identity was gated by the retained kernel owner. This is offline
		// decoding of its raw sample, not acquisition of these domains.
		valid = intel_n100_reset_domains(0x46d18086, 0, result.rawFuse, plan);
		printf("media_disable[9140]=%#x planned_domains=%u\n", result.rawFuse, plan.count);
		for (unsigned i = 0; valid && i < plan.count; i++)
			printf("domain=%u request=%#x ack=%#x\n", plan.domains[i].kind,
				plan.domains[i].request, plan.domains[i].ack);
	}
	puts("Fixed raw media-fuse observation and offline decode only; no all-domain wake, reset or GPU execution.");
	return valid ? 0 : 1;
}
