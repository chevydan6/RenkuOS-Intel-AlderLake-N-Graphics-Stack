// SPDX-License-Identifier: MIT
#include "BootFrameBufferInfo.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

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
	BootFrameBufferInfo info = {};
	Check(BuildBootFrameBufferInfo(0xfd000000, 1280, 800, 32, 5120, info),
		"valid QEMU boot descriptor");
	Check(info.version == 1 && info.size == 48 && info.reserved == 0
		&& info.flags == kBootFrameBufferSnapshot, "version and initialized fields");
	Check(info.physicalBase == 0xfd000000 && info.visibleBytes == 4096000
		&& info.width == 1280 && info.height == 800 && info.depth == 32
		&& info.bytesPerRow == 5120, "QEMU descriptor values");

	const struct {
		uint64_t base;
		int32_t width, height, depth, stride;
	} invalid[] = {
		{0, 0, 800, 32, 5120}, {0, -1, 800, 32, 5120},
		{0, 1280, 0, 32, 5120}, {0, 1280, -1, 32, 5120},
		{0, 1280, 800, 0, 5120}, {0, 1280, 800, -1, 5120},
		{0, 1280, 800, 33, 5120}, {0, 1280, 800, 32, 0},
		{0, 1280, 800, 32, -1}, {0, 1280, 800, 32, 5119},
		{UINT64_MAX - 4095999, 1280, 800, 32, 5120},
		{UINT64_MAX, 1, 1, 1, 1}, {0, INT_MAX, 1, 32, INT_MAX},
		{0, 9, 1, 1, 1}
	};
	for (const auto& input : invalid) {
		BootFrameBufferInfo before = info;
		Check(!BuildBootFrameBufferInfo(input.base, input.width, input.height,
			input.depth, input.stride, info), "invalid descriptor rejected");
		Check(memcmp(&info, &before, sizeof(info)) == 0,
			"failure preserves output");
	}
	Check(BuildBootFrameBufferInfo(UINT64_MAX - 4096000,
		1280, 800, 32, 5120, info), "non-wrapping address boundary");
	Check(BuildBootFrameBufferInfo(0, 9, 1, 1, 2, info)
		&& info.visibleBytes == 2, "packed monochrome rounds stride up");
	Check(BuildBootFrameBufferInfo(0, 1, INT_MAX, 32, INT_MAX, info)
		&& info.visibleBytes == uint64_t(INT_MAX) * INT_MAX,
		"large dimensions use 64-bit arithmetic");
	printf("%u boot framebuffer checks, %u failures\n", sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}
