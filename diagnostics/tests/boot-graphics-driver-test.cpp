// SPDX-License-Identifier: MIT
// Run only against the private diagnostic installed in a disposable std-VGA VM.
#include "BootGraphicsDriver.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

static unsigned sChecks;
static unsigned sFailures;

static void
Check(bool condition, const char* name)
{
	sChecks++;
	if (!condition) {
		sFailures++;
		printf("FAIL %s\n", name);
	}
}


int
main()
{
	int device = open(BOOT_GRAPHICS_DEVICE_PATH, O_RDWR);
	if (device < 0) {
		fprintf(stderr, "open diagnostic: %s\n", strerror(errno));
		return 1;
	}
	BootFrameBufferInfo info = {};
	Check(ioctl(device, BOOT_GRAPHICS_GET_BOOT_INFO, &info, sizeof(info)) == 0,
		"read kernel boot snapshot");
	Check(info.version == 1 && info.size == 48 && info.flags == 1
		&& info.reserved == 0, "versioned initialized ABI");
	Check(info.physicalBase == 0xfd000000 && info.visibleBytes == 4096000
		&& info.width == 1280 && info.height == 800 && info.depth == 32
		&& info.bytesPerRow == 5120, "expected std-VGA boot snapshot");

	const size_t sizes[] = {0, 1, 47, 49};
	for (size_t size : sizes) {
		BootFrameBufferInfo output;
		memset(&output, 0xa5, sizeof(output));
		BootFrameBufferInfo before = output;
		Check(ioctl(device, BOOT_GRAPHICS_GET_BOOT_INFO, &output, size) == -1
			&& errno == B_BAD_VALUE, "reject wrong ABI size");
		Check(memcmp(&output, &before, sizeof(output)) == 0,
			"wrong-size query preserves output");
	}
	Check(ioctl(device, BOOT_GRAPHICS_GET_BOOT_INFO + 1, &info, sizeof(info)) == -1
		&& errno == ENOTTY, "reject unknown operation");
		// The kernel fd layer translates B_DEV_INVALID_IOCTL to POSIX ENOTTY.
	Check(ioctl(device, BOOT_GRAPHICS_GET_BOOT_INFO, NULL, sizeof(info)) == -1
		&& errno == B_BAD_ADDRESS, "reject null output without panic");
	Check(ioctl(device, BOOT_GRAPHICS_GET_BOOT_INFO,
			reinterpret_cast<void*>(UINTPTR_MAX), sizeof(info)) == -1
		&& errno == B_BAD_ADDRESS, "reject wrapping kernel output");
	char byte = 0;
	Check(read(device, &byte, 1) == -1 && errno == B_NOT_ALLOWED,
		"reject stream reads");
	Check(write(device, &byte, 1) == -1 && errno == B_NOT_ALLOWED,
		"reject stream writes");

	for (unsigned i = 0; i < 64; i++) {
		int another = open(BOOT_GRAPHICS_DEVICE_PATH, O_RDONLY);
		BootFrameBufferInfo next = {};
		Check(another >= 0 && ioctl(another, BOOT_GRAPHICS_GET_BOOT_INFO,
			&next, sizeof(next)) == 0 && memcmp(&next, &info, sizeof(info)) == 0,
			"repeated opens preserve immutable snapshot");
		if (another >= 0)
			close(another);
	}
	fflush(stdout);
	pid_t child = fork();
	Check(child >= 0, "fork unprivileged access test");
	if (child == 0) {
		if (setuid(65534) != 0)
			_exit(2);
		BootFrameBufferInfo denied;
		bool inheritedDenied = ioctl(device, BOOT_GRAPHICS_GET_BOOT_INFO,
			&denied, sizeof(denied)) == -1 && errno == B_NOT_ALLOWED;
		int unprivileged = open(BOOT_GRAPHICS_DEVICE_PATH, O_RDONLY);
		bool openDenied = unprivileged < 0;
		if (unprivileged >= 0)
			close(unprivileged);
		close(device);
		_exit(inheritedDenied && openDenied ? 0 : 1);
	}
	if (child > 0) {
		int status;
		Check(waitpid(child, &status, 0) == child && WIFEXITED(status)
			&& WEXITSTATUS(status) == 0, "unprivileged open and inherited FD denied");
	}
	close(device);
	printf("%u native boot diagnostic checks, %u failures\n", sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}
