// SPDX-License-Identifier: MIT
#include "PrepareReport.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
int main()
{
	int fd = open("/dev/misc/intel_native_prepare", O_RDONLY);
	if (fd < 0) { perror("open native preparation bridge"); return 1; }
	native_gpu_prepare_report report = {};
	int status = ioctl(fd, NATIVE_GPU_PREPARE_IOCTL, &report, sizeof(report));
	close(fd);
	printf("native_prepare version=%u ioctl=%d mapping=%ld tables=%ld image=%ld cleanup=%ld\n",
		report.version, status, long(report.mapping), long(report.tables), long(report.image), long(report.cleanup));
	printf("pages=%u tail=%u padded_tail=%u ring_hash=%08x context_hash=%08x poison=%u gpu_submitted=%u\n",
		report.pages, report.ring_tail, report.workaround_tail, report.ring_checksum,
		report.context_checksum, report.result_poisoned, report.gpu_submitted);
	bool passed = status == 0 && report.version == 1 && report.mapping == 0
		&& report.tables == 0 && report.image == 0 && report.cleanup == 0 && report.pages == 9
		&& report.ring_tail == 96 && report.workaround_tail == 104
		&& report.result_poisoned == 1 && report.gpu_submitted == 0;
	puts(passed ? "Native owned request preparation: PASS (not GPU execution)" : "Native owned request preparation: FAIL");
	return passed ? 0 : 1;
}
