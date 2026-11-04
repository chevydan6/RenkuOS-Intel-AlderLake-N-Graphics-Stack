// SPDX-License-Identifier: MIT
#include "intel_gtt.h"

#include <stdio.h>


int
main()
{
	// Explicit datasheet values, including the zero-size encoding. No formula
	// from the decoder is reused to generate the expected results.
	const struct { unsigned code; unsigned mib; } valid[] = {
		{0x00, 0}, {0x01, 32}, {0x02, 64}, {0x03, 96}, {0x04, 128},
		{0x05, 160}, {0x06, 192}, {0x07, 224}, {0x08, 256},
		{0x09, 288}, {0x0a, 320}, {0x0b, 352}, {0x0c, 384},
		{0x0d, 416}, {0x0e, 448}, {0x0f, 480}, {0x10, 512},
		{0x20, 1024}, {0x30, 1536}, {0x40, 2048},
		{0xf0, 4}, {0xf1, 8}, {0xf2, 12}, {0xf3, 16}, {0xf4, 20},
		{0xf5, 24}, {0xf6, 28}, {0xf7, 32}, {0xf8, 36},
		{0xf9, 40}, {0xfa, 44}, {0xfb, 48}, {0xfc, 52},
		{0xfd, 56}, {0xfe, 60}
	};
	uint64_t expected[256];
	for (unsigned i = 0; i < 256; i++)
		expected[i] = UINT64_MAX;
	for (unsigned i = 0; i < sizeof(valid) / sizeof(valid[0]); i++)
		expected[valid[i].code] = (uint64_t)valid[i].mib << 20;

	unsigned failures = 0;
	for (unsigned config = 0; config <= 0xffff; config++) {
		uint64_t size = UINT64_MAX;
		bool decoded = intel_alderlake_stolen_size(config, &size);
		bool wanted = expected[config >> 8] != UINT64_MAX;
		if (decoded != wanted || size != expected[config >> 8]) {
			fprintf(stderr, "FAIL GGC 0x%04x\n", config);
			failures++;
		}
	}
	if (intel_alderlake_stolen_size(0x0200, NULL))
		failures++;
	printf("65537 GGC decode checks, %u failures\n", failures);
	return failures == 0 ? 0 : 1;
}
