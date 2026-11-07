// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_BCS_INVENTORY_H
#define INTEL_N100_BCS_INVENTORY_H
#include <stdint.h>
#include <stddef.h>

// Raw sequential samples only: no engine-present, idle or submission-ready
// conclusion. Owner supplies live GT forcewake, UC mapping and per-read guards.
struct IntelN100BcsInventory {
	uint32_t tail, head, control, mode, pendingWake;
};
static const uint32_t kIntelN100BcsOffsets[5] = {
	0x22030, 0x22034, 0x2203c, 0x2209c, 0x800c
};
static inline bool
intel_n100_sample_bcs(bool (*read)(void*, uint32_t, uint32_t&), void* context,
	IntelN100BcsInventory& output)
{
	if (read == NULL)
		return false;
	uint32_t values[5] = {};
	for (unsigned i = 0; i < 5; i++) {
		if (!read(context, kIntelN100BcsOffsets[i], values[i])
			|| values[i] == UINT32_MAX)
			return false;
	}
	IntelN100BcsInventory sample = {
		values[0], values[1], values[2], values[3], values[4]
	};
	output = sample;
	return true;
}
#endif
