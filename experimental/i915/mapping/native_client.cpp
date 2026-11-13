// SPDX-License-Identifier: MIT
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "native_probe.h"
int main()
{
	alarm(20);
	int fd = open("/dev/misc/i915_mapping_test", O_RDONLY);
	if (fd < 0) { perror("open"); return 1; }
	puts("evidence=NATIVE_GGTT_MEMORY_ONLY gpu_submission=false cpu_stores=true");
	bool passed = true;
	for (unsigned run = 0; run < 4 && passed; run++) {
		i915_mapping_result r = {};
		if (ioctl(fd, I915_MAPPING_TEST_IOCTL, &r, sizeof(r)) < 0) {
			perror("mapping ioctl"); passed = false; break;
		}
		printf("run=%u version=%u module=%d begin=%d end=%d checks=%u failures=%u\n",
			run, r.version, (int)r.module_status, (int)r.begin_status,
			(int)r.end_status, r.checks, r.failures);
		for (unsigned i = 0; i < 4; i++)
			printf("slot=%u ggtt_offset=%#" PRIx64 " bytes=%" PRIu64 "\n",
				i, r.ggtt[i], r.bytes[i]);
		passed = r.version == 1 && r.module_status == B_OK && r.begin_status == B_OK
			&& r.end_status == B_OK && r.checks == 38927 && r.failures == 0;
		fflush(stdout);
	}
	close(fd);
	puts(passed ? "PASS owned GGTT aperture CPU readback and release; no GPU execution"
		: "FAIL mapping diagnostic; preserve resident owner state and collect logs");
	return passed ? 0 : 1;
}
