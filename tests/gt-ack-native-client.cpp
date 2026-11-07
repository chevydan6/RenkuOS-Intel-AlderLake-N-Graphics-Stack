// SPDX-License-Identifier: MIT
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "gt-ack-native-protocol.h"

int main()
{
	alarm(10); // Userland only: cannot interrupt a stuck kernel MMIO load.
	int fd = open("/dev/misc/intel_gt_ack", O_RDONLY);
	if (fd < 0) { perror("open"); return 1; }
	bool valid = true;
	for (unsigned i = 0; i < 2; i++) {
		GtAckResult result = {};
		if (ioctl(fd, GT_ACK_IOCTL, &result, sizeof(result)) < 0) {
			perror("observe"); close(fd); return 1;
		}
		printf("run=%u version=%u module=%d observation=%d "
			"offset=%#x raw=%#x/%#x kernel=%u/%u fallback=%u/%u\n", i,
			result.version, int(result.module), int(result.observation),
			kIntelN100GtAckOffset, result.sample.first, result.sample.second,
			result.sample.first & 1, result.sample.second & 1,
			(result.sample.first >> 15) & 1, (result.sample.second >> 15) & 1);
		valid = valid && result.version == 1 && result.module == B_OK
			&& result.observation == B_OK && result.sample.first != UINT32_MAX
			&& result.sample.second != UINT32_MAX;
		if (!valid) break;
	}
	close(fd);
	puts("Raw read-only ACK samples; no forcewake request, GPU workload or power hold.");
	return valid ? 0 : 1;
}
