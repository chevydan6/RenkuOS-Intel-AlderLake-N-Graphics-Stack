// SPDX-License-Identifier: MIT
#include "IntelN100Ppgtt.h"
#include <assert.h>
#include <stdio.h>

int main()
{
	// Literal expected flags, independent of the encoder's shifts.
	const uint64_t patBits[] = {0, 8, 16, 24, 128, 136, 144, 152};
	const uint64_t pages[] = {0, 4096, 1ULL << 32, (1ULL << 39) - 4096};
	unsigned checks = 0;
	for (uint64_t page : pages) {
		for (unsigned pat = 0; pat < 8; pat++) {
			for (unsigned rw = 0; rw < 2; rw++) {
				uint64_t entry = UINT64_MAX;
				assert(intel_n100_ppgtt_leaf(page, pat, rw, &entry));
				assert(entry == (page | patBits[pat] | (rw ? 3 : 1)));
				checks++;
			}
		}
		uint64_t entry;
		assert(intel_n100_ppgtt_directory(page, IntelN100DirectoryCached, &entry));
		assert(entry == (page | 3));
		assert(intel_n100_ppgtt_directory(page, IntelN100DirectoryUncached, &entry));
		assert(entry == (page | 0x1b));
		checks += 2;
	}
	const uint64_t invalidPages[] = {1, 4095, (1ULL << 39) - 1, 1ULL << 39, UINT64_MAX};
	for (uint64_t page : invalidPages) {
		uint64_t entry = 0xfeed;
		assert(!intel_n100_ppgtt_leaf(page, 0, true, &entry) && entry == 0xfeed);
		assert(!intel_n100_ppgtt_directory(page, IntelN100DirectoryCached, &entry)
			&& entry == 0xfeed);
		checks += 2;
	}
	uint64_t entry = 0xfeed;
	assert(!intel_n100_ppgtt_leaf(0, 8, true, &entry) && entry == 0xfeed);
	assert(!intel_n100_ppgtt_leaf(0, ~0u, true, &entry) && entry == 0xfeed);
	assert(!intel_n100_ppgtt_directory(0, static_cast<IntelN100DirectoryCache>(2), &entry)
		&& entry == 0xfeed);
	assert(!intel_n100_ppgtt_directory(0, static_cast<IntelN100DirectoryCache>(255), &entry)
		&& entry == 0xfeed);
	assert(!intel_n100_ppgtt_leaf(0, 0, true, NULL));
	assert(!intel_n100_ppgtt_directory(0, IntelN100DirectoryCached, NULL));
	checks += 6;
	for (unsigned level = 0; level < 4; level++) {
		for (unsigned index = 0; index < 512; index++) {
			uint16_t indices[4] = {};
			assert(intel_n100_ppgtt_indices((uint64_t)index << (39 - level * 9), indices));
			for (unsigned other = 0; other < 4; other++)
				assert(indices[other] == (other == level ? index : 0));
			checks++;
		}
	}
	uint16_t indices[4] = {};
	assert(intel_n100_ppgtt_indices((1ULL << 39) | (2ULL << 30)
		| (3ULL << 21) | (4ULL << 12) | 4095, indices));
	for (unsigned i = 0; i < 4; i++) assert(indices[i] == i + 1);
	checks++;
	assert(intel_n100_ppgtt_indices((1ULL << 48) - 1, indices));
	for (unsigned i = 0; i < 4; i++) assert(indices[i] == 511);
	assert(!intel_n100_ppgtt_indices(1ULL << 48, indices));
	for (unsigned i = 0; i < 4; i++) assert(indices[i] == 511);
	assert(!intel_n100_ppgtt_indices(UINT64_MAX, indices));
	assert(!intel_n100_ppgtt_indices(0, NULL));
	checks += 4;
	assert(intel_n100_ppgtt_range(0, 4096));
	assert(intel_n100_ppgtt_range(0, 1ULL << 48));
	assert(intel_n100_ppgtt_range((1ULL << 48) - 4096, 4096));
	assert(!intel_n100_ppgtt_range((1ULL << 48) - 4096, 8192));
	assert(!intel_n100_ppgtt_range(1ULL << 48, 4096));
	assert(!intel_n100_ppgtt_range(0, 0));
	assert(!intel_n100_ppgtt_range(1, 4096));
	assert(!intel_n100_ppgtt_range(0, 4095));
	assert(!intel_n100_ppgtt_range(4096, UINT64_MAX - 4095));
	checks += 9;
	printf("%u PPGTT encoding/range cases passed; offline only, no GPU access.\n", checks);
}
