/* SPDX-License-Identifier: MIT */
#include "native_test_ioctl.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	unsigned int repeat = 8;
	if (argc == 3 && strcmp(argv[1], "--repeat") == 0) repeat = strtoul(argv[2], NULL, 10);
	else if (argc != 1) return 2;
	if (repeat == 0 || repeat > 1000) return 2;
	int fd = open(KPI_TEST_PATH, O_RDWR);
	if (fd < 0) { perror(KPI_TEST_PATH); return 1; }
	struct kpi_test_report report;
	if (ioctl(fd, KPI_RUN_SELFTEST + 1000, &report, sizeof(report)) >= 0 ||
		ioctl(fd, KPI_RUN_SELFTEST, &report, 1) >= 0 ||
		ioctl(fd, KPI_RUN_SELFTEST, NULL, sizeof(report)) >= 0) {
		fprintf(stderr, "FAIL invalid ioctl accepted\n"); close(fd); return 1;
	}
	puts("evidence=NATIVE_KERNEL_CPU_SELFTEST");
	puts("gpu_submission=false");
	puts("PASS invalid operation/length/user-buffer controls");
	for (unsigned int i = 0; i < repeat; ++i) {
		memset(&report, 0xa5, sizeof(report));
		if (ioctl(fd, KPI_RUN_SELFTEST, &report, sizeof(report)) < 0) {
			perror("kernel selftest ioctl"); close(fd); return 1;
		}
		printf("run=%u checks=%u failures=%u first_failure_line=%u pages=%u/%u workers=%u iterations=%u final_releases=%u timeout_status=%" B_PRId32 " timeout_us=%" B_PRIu64 " elapsed_us=%" B_PRIu64 "\n",
			i, (unsigned)report.checks, (unsigned)report.failures,
			(unsigned)report.first_failure_line, (unsigned)report.allocated_pages,
			(unsigned)report.freed_pages, (unsigned)report.kernel_threads,
			(unsigned)report.worker_iterations, (unsigned)report.final_releases,
			report.timeout_status, report.timeout_elapsed_us, report.elapsed_us);
		if (report.abi_version != 1 || report.failures || report.checks < 20 ||
			report.allocated_pages != 4 || report.freed_pages != 4 ||
			report.kernel_threads != 2 || report.worker_iterations != 4000 ||
			report.final_releases != 1 || report.timeout_elapsed_us < 20000) {
			close(fd); return 1;
		}
	}
	close(fd);
	puts("PASS native kernel CPU services; no GPU execution tested");
	return 0;
}
