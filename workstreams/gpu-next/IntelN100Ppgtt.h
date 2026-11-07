// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_PPGTT_H
#define INTEL_N100_PPGTT_H

#include <stddef.h>
#include <stdint.h>

// Offline 4 KiB system-memory encoding candidate for 8086:46d1.
// Does not allocate, install, invalidate or publish GPU page tables. PAT indices
// are raw selectors: callers must establish their programmed cache policy before
// hardware use. A clear RW bit alone is not proof of enforced write protection.
// Format reference: Linux 028ef9c96e96197026887c0f092424679298aae8,
// drivers/gpu/drm/i915/gt/{intel_gtt.h,gen8_ppgtt.c}, MIT.

static inline bool
intel_n100_ppgtt_page(uint64_t physical)
{
	return (physical & 4095) == 0 && physical <= (1ULL << 39) - 4096;
}

static inline bool
intel_n100_ppgtt_leaf(uint64_t physical, unsigned patIndex, bool writable,
	uint64_t* entry)
{
	if (entry == NULL || !intel_n100_ppgtt_page(physical) || patIndex > 7)
		return false;
	uint64_t value = physical | 1;
	if (writable)
		value |= 1ULL << 1;
	value |= (uint64_t)(patIndex & 1) << 3;
	value |= (uint64_t)(patIndex & 2) << 3;
	value |= (uint64_t)(patIndex & 4) << 5;
	// LM, 64 KiB, 2 MiB and later-generation PAT bits are deliberately absent.
	*entry = value;
	return true;
}

enum IntelN100DirectoryCache : uint8_t {
	IntelN100DirectoryCached,
	IntelN100DirectoryUncached
};

static inline bool
intel_n100_ppgtt_directory(uint64_t physical, IntelN100DirectoryCache cache,
	uint64_t* entry)
{
	if (entry == NULL || !intel_n100_ppgtt_page(physical)
		|| (cache != IntelN100DirectoryCached && cache != IntelN100DirectoryUncached))
		return false;
	*entry = physical | 3 | (cache == IntelN100DirectoryUncached ? 0x18 : 0);
	return true;
}

// These are unsigned 48-bit GPU addresses, never sign-extended CPU pointers.
// Output order is root to leaf: PML4E, PDPE, PDE, PTE. Byte offsets are allowed
// for index lookup; callers creating mappings must use the aligned range check.
static inline bool
intel_n100_ppgtt_indices(uint64_t gpuAddress, uint16_t* indices)
{
	if (indices == NULL || gpuAddress >= (1ULL << 48))
		return false;
	for (unsigned level = 0; level < 4; level++)
		indices[level] = (gpuAddress >> (39 - 9 * level)) & 0x1ff;
	return true;
}

static inline bool
intel_n100_ppgtt_range(uint64_t start, uint64_t length)
{
	const uint64_t limit = 1ULL << 48;
	return start < limit && (start & 4095) == 0 && length != 0
		&& (length & 4095) == 0 && length <= limit - start;
}
#endif
