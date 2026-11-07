// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_NONCE_STORES_H
#define INTEL_N100_NONCE_STORES_H
#include <stddef.h>
#include <stdint.h>

// Offline payload encoder for the first GGTT nonce/complement experiment.
// resultPage is a GGTT OFFSET, never a CPU pointer or physical address. The
// caller must prove exclusive ownership of a pinned mapped 4K result page.
// Below4GiB is this experiment's restriction, not a hardware GGTT width claim.
// Reference: Linux028ef9c96e96197026887c0f092424679298aae8 i915
// intel_gpu_commands.h and selftest_lrc.c MI_STORE_DWORD_IMM_GEN4|MI_USE_GGTT.
// These8dwords are only the payload: no flush, completion, ring management,
// publication or retirement. They must not be submitted as a complete request.
static inline bool
intel_n100_ggtt_nonce_stores(uint64_t resultPage, uint32_t nonce,
	uint32_t* words, size_t capacity)
{
	if (words == NULL || capacity < 8 || (resultPage & 4095) != 0
		|| resultPage > 0xfffff000ULL)
		return false;
	const uint32_t command = 0x10400002;
	words[0] = command;
	words[1] = (uint32_t)resultPage;
	words[2] = 0;
	words[3] = nonce;
	words[4] = command;
	words[5] = (uint32_t)resultPage + 4;
	words[6] = 0;
	words[7] = ~nonce;
	return true;
}
#endif
