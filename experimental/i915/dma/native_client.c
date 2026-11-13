/* SPDX-License-Identifier: MIT */
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "native_test.h"
int main(void)
{
	alarm(20);
	int fd = open("/dev/misc/i915_linux_dma_test", O_RDONLY);
	if (fd < 0) { perror("open"); return 1; }
	puts("evidence=LINUX_DMA_API_NATIVE_BACKEND gpu_submission=false");
	int failed = 0;
	for (unsigned i = 0; i < 4; i++) {
		struct linux_dma_result result = {};
		if (ioctl(fd, LINUX_DMA_IOCTL, &result, sizeof(result)) < 0) { perror("ioctl"); failed = 1; break; }
		printf("run=%u checks=%u failures=%u coherent=%u/%u sg=%u/%u\n",i,
			result.checks,result.failures,result.coherent_allocations,result.coherent_frees,
			result.streaming_maps,result.streaming_unmaps);
		fflush(stdout);
		if (result.version != 1 || result.failures || result.coherent_allocations != 4
			|| result.coherent_frees != 4 || result.streaming_maps != 4 || result.streaming_unmaps != 4) {
			failed = 1; break;
		}
	}
	close(fd);
	puts(failed ? "FAIL Linux DMA API" : "PASS Linux DMA API on native bus_dma; no GPU transfer tested");
	return failed;
}
