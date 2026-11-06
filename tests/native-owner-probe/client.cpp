// SPDX-License-Identifier: MIT
// Disposable-VM client for terra_owner_probe; no GPU access.
#include <OS.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "protocol.h"
int main()
{
	alarm(15);
	int fd = open("/dev/misc/terra_owner_probe", O_RDWR);
	if (fd < 0) { perror("open"); return 1; }
	terra_probe_result result = {};
	if (ioctl(fd, TERRA_PROBE_GET, &result, sizeof(result)) != B_OK) {
		perror("probe ioctl"); close(fd); return 1;
	}
	printf("module=%ld nonexistent-pci=%ld outputs-unchanged=%lu area=%ld\n",
		(long)result.module_status, (long)result.begin_status,
		(unsigned long)result.outputs_unchanged, (long)result.area);
	if (result.module_status != B_OK || result.begin_status != B_NOT_SUPPORTED
		|| !result.outputs_unchanged) { close(fd); return 1; }
	void* address = NULL;
	area_id clone = clone_area("terra owner user clone", &address,
		B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, result.area);
	if (clone < B_OK) { fprintf(stderr, "clone failed: %ld\n", (long)clone); close(fd); return 1; }
	volatile uint32* word = (volatile uint32*)address;
	bool ok = *word == TERRA_PROBE_SENTINEL;
	*word = TERRA_PROBE_USER_VALUE;
	ok = ioctl(fd, TERRA_PROBE_CHECK, NULL, 0) == B_OK && ok;
	ok = close(fd) == 0 && ok;
	area_info originalInfo;
	ok = get_area_info(result.area, &originalInfo) < B_OK && ok;
	// close releases the driver cookie and original kernel area; this clone
	// must still own its backing. No physical device registers are accessed.
	ok = *word == TERRA_PROBE_USER_VALUE && ok;
	ok = delete_area(clone) == B_OK && ok;
	alarm(0);
	puts(ok ? "PASS: actual owner rejected impossible PCI; RAM kernel clone survived close"
		: "FAIL: owner admission or RAM clone lifetime");
	return ok ? 0 : 1;
}
