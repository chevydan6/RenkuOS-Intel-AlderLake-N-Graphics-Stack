// SPDX-License-Identifier: MIT
// Real allocation copy-out rollback probe. No GPU commands or mapping writes.
#include <OS.h>
#include <AGP.h>
#include <intel_extreme.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	setvbuf(stdout, nullptr, _IOLBF, 0);
	if (argc != 3 || strcmp(argv[1], "--run") != 0
		|| strncmp(argv[2], "/dev/graphics/", 14) != 0) {
		puts("memory-copyout --run /dev/graphics/DEVICE");
		puts("One read-only copy-out failure, then one ordinary allocation/free.");
		puts("Keep app_server open and observe externally for 60 seconds.");
		return argc == 2 && strcmp(argv[1], "--help") == 0 ? 0 : 2;
	}
	int fd = open(argv[2], O_RDWR);
	if (fd < 0) { perror("open"); return 1; }
	intel_get_shared_info_layout layout = {};
	layout.magic = INTEL_PRIVATE_DATA_MAGIC;
	if (ioctl(fd, INTEL_GET_SHARED_INFO_LAYOUT, &layout, sizeof(layout)) < 0
		|| layout.version != INTEL_SHARED_INFO_LAYOUT_VERSION
		|| layout.shared_info_size != sizeof(intel_shared_info)) {
		puts("Interface check failed"); close(fd); return 1;
	}
	void* page = nullptr;
	area_id area = create_area("allocation copyout probe", &page, B_ANY_ADDRESS,
		B_PAGE_SIZE, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < 0) { close(fd); return 1; }
	intel_allocate_graphics_memory request = {};
	request.magic = INTEL_PRIVATE_DATA_MAGIC;
	request.size = B_PAGE_SIZE;
	request.alignment = B_PAGE_SIZE;
	request.flags = B_APERTURE_NON_RESERVED;
	memcpy(page, &request, sizeof(request));
	if (set_area_protection(area, B_READ_AREA) != B_OK) {
		delete_area(area); close(fd); return 1;
	}
	// The kernel can read the request but cannot return its allocation result.
	// Never guess the hidden descriptor or attempt to free it from userspace.
	errno = 0;
	int result = ioctl(fd, INTEL_ALLOCATE_GRAPHICS_MEMORY, page, sizeof(request));
	int error = errno;
	bool unchanged = memcmp(page, &request, sizeof(request)) == 0;
	printf("readonly ioctl=%d errno=%d expected=%d unchanged=%d\n",
		result, error, int(B_BAD_ADDRESS), int(unchanged));
	delete_area(area);
	if (result >= 0 || error != B_BAD_ADDRESS || !unchanged) {
		puts("Unexpected result; stop and collect logs. No guessed cleanup.");
		close(fd); return 1;
	}
	if (ioctl(fd, INTEL_ALLOCATE_GRAPHICS_MEMORY, &request, sizeof(request)) < 0) {
		perror("follow-up allocation"); close(fd); return 1;
	}
	intel_free_graphics_memory release = {};
	release.magic = INTEL_PRIVATE_DATA_MAGIC;
	release.buffer_base = request.buffer_base;
	if (ioctl(fd, INTEL_FREE_GRAPHICS_MEMORY, &release, sizeof(release)) < 0) {
		perror("follow-up free"); close(fd); return 1;
	}
	close(fd);
	puts("Expected copyout error and follow-up allocation/free passed.");
	puts("Not a resource-count, close-race or GPU-execution proof.");
	return 0;
}
