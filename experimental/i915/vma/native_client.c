/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "test.h"
int main(void)
{
	alarm(60);
	int fd = open("/dev/misc/i915_linux_vma_test", O_RDONLY);
	if (fd < 0) { perror("open"); return 1; }
	puts("evidence=WHOLE_DRM_VMA_NATIVE_CPU_TEST gpu_submission=false");
	int failed = 0;
	for (unsigned i = 0; i < 4; i++) {
		struct vma_native_report r = {};
		if (ioctl(fd, VMA_IOCTL, &r, sizeof(r)) < 0) { perror("ioctl"); failed = 1; break; }
		printf("run=%u checks=%u failures=%u first_line=%u iterations=%u concurrent_readers=%u worker_errors=%u\n",
			i, r.result.checks, r.result.failures, r.result.first_line, r.iterations, r.max_readers, r.worker_errors);
		fflush(stdout);
		if (r.version != 1 || r.result.checks < 500 || r.result.failures
			|| r.iterations != 12000 || r.max_readers != 2 || r.worker_errors) { failed = 1; break; }
	}
	close(fd);
	return failed;
}
