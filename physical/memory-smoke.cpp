// SPDX-License-Identifier: MIT
// Allocation/free diagnostic only; no GPU command submission.
#include <OS.h>
#include <intel_extreme.h>
#include <AGP.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int
main(int argc, char** argv)
{
	if (argc == 2 && strcmp(argv[1], "--help") == 0) {
		puts("Usage: memory-smoke --run /dev/graphics/DEVICE");
		puts("Requires reviewed artifacts and verified external recovery.");
		puts("30 private non-reserved allocations/frees; no GPU workload.");
		return 0;
	}
	if (argc != 3 || strcmp(argv[1], "--run") != 0
		|| strncmp(argv[2], "/dev/graphics/", 14) != 0)
		return 2;

	// A user alarm cannot bound an uninterruptible kernel ioctl.
	// External recovery remains mandatory for the physical test.
	alarm(20);
	int fd = open(argv[2], O_RDWR);
	if (fd < 0) {
		perror("open");
		return 1;
	}
	intel_get_shared_info_layout layout = {};
	layout.magic = INTEL_PRIVATE_DATA_MAGIC;
	if (ioctl(fd, INTEL_GET_SHARED_INFO_LAYOUT, &layout, sizeof(layout)) < 0
		|| layout.version != INTEL_SHARED_INFO_LAYOUT_VERSION
		|| layout.shared_info_size != sizeof(intel_shared_info)) {
		fprintf(stderr, "Interface mismatch/query failure; stopping.\n");
		close(fd);
		return 1;
	}
	const uint32 sizes[] = {4096, 8192, 65536};
	for (unsigned round = 0; round < 30; round++) {
		intel_allocate_graphics_memory allocation = {};
		allocation.magic = INTEL_PRIVATE_DATA_MAGIC;
		allocation.size = sizes[round % 3];
		allocation.alignment = B_PAGE_SIZE;
		allocation.flags = B_APERTURE_NON_RESERVED;
		if (ioctl(fd, INTEL_ALLOCATE_GRAPHICS_MEMORY, &allocation,
				sizeof(allocation)) < 0) {
			fprintf(stderr, "Allocation round %u failed: %s; no retry.\n",
				round, strerror(errno));
			close(fd);
			return 1;
		}
		intel_free_graphics_memory release = {};
		release.magic = INTEL_PRIVATE_DATA_MAGIC;
		release.buffer_base = allocation.buffer_base;
		if (ioctl(fd, INTEL_FREE_GRAPHICS_MEMORY, &release,
				sizeof(release)) < 0) {
			fprintf(stderr, "Free round %u failed: %s; retained base %#"
				B_PRIxADDR ". Stop and collect logs.\n", round,
				strerror(errno), allocation.buffer_base);
			close(fd);
			return 1;
		}
		printf("round=%u bytes=%" B_PRIu32 " allocation/free passed\n",
			round, allocation.size);
		fflush(stdout);
	}
	close(fd);
	alarm(0);
	puts("30 rounds passed; not GPU execution proof.");
	return 0;
}
