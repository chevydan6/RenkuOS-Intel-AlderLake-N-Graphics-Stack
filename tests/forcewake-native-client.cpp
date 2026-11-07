// SPDX-License-Identifier: MIT
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "forcewake-native-protocol.h"
int main()
{
	alarm(10); // Bounds userland only; cannot interrupt stuck kernel MMIO.
	int fd = open("/dev/misc/intel_forcewake", O_RDONLY);
	if (fd < 0) { perror("open"); return 1; }
	ForcewakeResult result = {};
	int status = ioctl(fd, FORCEWAKE_IOCTL, &result, sizeof(result));
	if (status < 0) perror("handshake");
	close(fd);
	if (status < 0) return 1;
	const IntelN100Forcewake::Report& r = result.report;
	printf("version=%u module=%d transaction=%d error=%u cleanup=%u stage=%u ownership=%u\n",
		result.version, int(result.module), int(result.transaction), r.error, r.cleanup, r.stage, r.ownership);
	printf("initial=%#x set=%#x final=%#x last=%#x reads=%u writes=%u elapsed_us=%" PRIu64 "\n",
		r.initialAck, r.setAck, r.finalAck, r.lastAck, r.reads, r.writes, r.elapsed);
	puts("Standalone GT forcewake handshake; no engine commands, DMA or rendering.");
	bool valid = result.version == 1 && result.module == B_OK && result.transaction == B_OK
		&& r.error == 0 && r.cleanup == 0 && r.stage == IntelN100Forcewake::Complete
		&& r.ownership == IntelN100Forcewake::Released && r.writes == 2
		&& (r.initialAck & 0x8001) == 0 && (r.setAck & 0x8001) == 1
		&& (r.finalAck & 0x8001) == 0;
	return valid ? 0 : 1;
}
