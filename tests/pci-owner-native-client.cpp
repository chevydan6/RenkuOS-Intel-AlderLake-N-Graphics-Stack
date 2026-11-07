// SPDX-License-Identifier: MIT
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "pci-owner-native-protocol.h"

int main()
{
	alarm(10); // Userland bound only; cannot interrupt a stuck kernel PCI read.
	int fd = open("/dev/misc/intel_pci_owner", O_RDONLY);
	if (fd < 0) { perror("open"); return 1; }
	PciOwnerResult result = {};
	int status = ioctl(fd, PCI_OWNER_IOCTL, &result, sizeof(result));
	if (status < 0) perror("observe");
	close(fd);
	if (status < 0) return 1;
	printf("version=%u module=%d completed=%u\n", result.version,
		int(result.module), result.completed);
	bool valid = result.version == 1 && result.module == B_OK && result.completed == 2;
	for (unsigned i = 0; i < 2; i++) {
		const PciOwnerRun& run = result.runs[i];
		const IntelN100PciSnapshot& s = run.snapshot;
		printf("run=%u lease=%" PRIu64 " begin=%d duplicate=%d end=%d stale=%d "
			"id=%#x class_rev=%#x command=%#x BAR=%#x:%08x PM=%#x PMCSR=%#x\n",
			i, run.lease, int(run.begin), int(run.duplicate), int(run.end),
			int(run.stale), s.identity, s.classRevision, s.command,
			s.barHigh, s.barLow, s.pmOffset, s.pmControl);
		valid = valid && run.lease != 0 && run.begin == B_OK && run.duplicate == B_BUSY
			&& run.end == B_OK && run.stale == B_BAD_VALUE && s.identity == 0x46d18086
			&& s.classRevision == 0x03000000 && (s.command & 2) && (s.pmControl & 3) == 0;
	}
	valid = valid && result.runs[1].lease > result.runs[0].lease;
	puts("Read-only controlled-boot PCI observation; no GT writes or GPU execution.");
	return valid ? 0 : 1;
}
