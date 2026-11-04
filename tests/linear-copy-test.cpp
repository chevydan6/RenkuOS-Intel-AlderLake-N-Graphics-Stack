// SPDX-License-Identifier: MIT
#include "../experimental/gen12/LinearCopy.h"

#include <stdio.h>
#include <string.h>

using namespace Gen12Experiment;

static unsigned sChecks;
static unsigned sFailures;

static void
Check(bool condition, const char* description)
{
	sChecks++;
	if (!condition) {
		printf("FAIL %s\n", description);
		sFailures++;
	}
}

static void
Reject(const LinearSurface& destination, const LinearSurface& source,
	uint32_t width, uint32_t height, size_t capacity = kCopyDwords)
{
	uint32_t output[12];
	uint32_t original[12];
	memset(output, 0xa5, sizeof(output));
	memcpy(original, output, sizeof(output));
	Check(!PackLinearCopy32(destination, source, width, height, output, capacity),
		"invalid request rejected");
	Check(memcmp(output, original, sizeof(output)) == 0,
		"rejection leaves output unchanged");
}

int
main()
{
	LinearSurface destination = {0x100000, 65536, 2048};
	LinearSurface source = {0x200000, 65536, 2048};
	uint32_t output[12] = {};
	output[10] = output[11] = 0xabcdef01;
	Check(PackLinearCopy32(destination, source, 512, 32, output, 10),
		"Linux selftest linear geometry accepted");
	// Fixed vector independently transcribed from prepare_blit's linear
	// 512x32 branch; only the ten copy words, excluding its register setup.
	const uint32_t expected[10] = {0x50800008, 0x03000800, 0, 0x00200200,
		0x00100000, 0, 0, 0x800, 0x00200000, 0};
	Check(memcmp(output, expected, sizeof(expected)) == 0, "reference vector");
	Check(output[10] == 0xabcdef01 && output[11] == 0xabcdef01, "output canaries");
	Check(!PackLinearCopy32(destination, source, 1, 1, NULL, 10), "null output");
	Reject(destination, source, 1, 1, 9);
	Reject(destination, source, 0, 1);
	Reject(destination, source, 1, 0);
	Reject(destination, source, UINT32_MAX, 1);
	Reject(destination, source, 1, UINT32_MAX);
	Reject(destination, source, 513, 1);
	Reject(destination, source, 512, 33);
	Reject(destination, destination, 1, 1);
	for (unsigned side = 0; side < 2; side++) {
		LinearSurface cases[] = {
			{0, 65536, 2048}, {0x300001, 65536, 2048},
			{kAddressLimit, 65536, 2048}, {UINT64_MAX, 65536, 2048},
			{kAddressLimit - 4096, 4097, 2048},
			{0x300000, UINT64_MAX, 2048}, {0x300000, 0, 2048},
			{0x300000, 65536, 0}, {0x300000, 65536, 32768},
			{0x300000, 65536, 2047}, {0x300000, 65535, 2048},
			{0x101000, 65536, 2048}
		};
		for (const LinearSurface& bad : cases) {
			if (side == 0)
				Reject(bad, destination, 512, 32);
			else
				Reject(destination, bad, 512, 32);
		}
	}

	// Deterministic geometries with varied row padding and high address bits.
	// Decode fields independently and check the last byte of each rectangle.
	uint32_t state = 17;
	for (unsigned i = 0; i < 10000; i++) {
		state = state * 1664525U + 1013904223U;
		uint32_t width = state % 8191 + 1;
		uint32_t height = (state >> 16) % 32767 + 1;
		uint32_t pitch = (width + (state % (8192 - width))) * 4;
		uint64_t bytes = uint64_t(height - 1) * pitch + width * 4;
		LinearSurface dst = {0x100000000ULL, bytes, pitch};
		LinearSurface src = {kAddressLimit - 0x80000000ULL, bytes, pitch};
		Check(PackLinearCopy32(dst, src, width, height, output, 10),
			"generated valid geometry");
		Check(output[0] == 0x50800008 && output[1] >> 24 == 3
			&& (output[1] & 0x00ff0000) == 0 && output[2] == 0
			&& output[6] == 0 && (output[7] >> 16) == 0,
			"opcode depth and unused fields");
		Check((output[3] & 0xffff) == width && output[3] >> 16 == height
			&& (output[1] & 0xffff) == pitch && output[7] == pitch,
			"decoded geometry");
		Check((uint64_t(output[5]) << 32 | output[4]) == dst.gpuAddress
			&& (uint64_t(output[9]) << 32 | output[8]) == src.gpuAddress,
			"decoded 64-bit addresses");
		dst.allocationBytes--;
		Reject(dst, src, width, height);
	}

	LinearSurface last = {kAddressLimit - 4096, 4096, 4};
	LinearSurface first = {4096, 4096, 4};
	Check(PackLinearCopy32(last, first, 1, 1024, output, 10), "last allowed page");
	LinearSurface adjacent = {8192, 4096, 4};
	Check(PackLinearCopy32(adjacent, first, 1, 1024, output, 10),
		"adjacent allocations allowed");
	printf("%u offline copy-packet checks, %u failures; no GPU submission\n",
		sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}
