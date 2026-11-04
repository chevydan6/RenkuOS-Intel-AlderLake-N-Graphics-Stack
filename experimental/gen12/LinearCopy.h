/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 chevydan6
 * Packet layout derived from Intel's MIT-licensed Linux reference:
 * Copyright © 2003-2018, 2019 Intel Corporation
 * See experimental/gen12/LICENSE.intel for the retained license notice.
 *
 * Offline packet experiment. No submission or MMIO interface is provided.
 * Field layout reference: Linux v7.0 i915_gem_client_blt.c::prepare_blit()
 * and intel_gpu_commands.h. See BLIT_RESEARCH.md for provenance and limits.
 */
#ifndef RENKU_EXPERIMENTAL_LINEAR_COPY_H
#define RENKU_EXPERIMENTAL_LINEAR_COPY_H

#include <stddef.h>
#include <stdint.h>

namespace Gen12Experiment {

struct LinearSurface {
	uint64_t gpuAddress;
	uint64_t allocationBytes;
	uint32_t pitchBytes;
};

// Conservative experimental policy, not the hardware's complete VA range.
static const uint64_t kAddressLimit = uint64_t(1) << 39;
static const size_t kCopyDwords = 10;

inline bool
ValidateSurface(const LinearSurface& surface, uint32_t width, uint32_t height)
{
	if (surface.gpuAddress == 0 || (surface.gpuAddress & 4095) != 0
		|| surface.gpuAddress >= kAddressLimit
		|| surface.allocationBytes == 0
		|| surface.allocationBytes > kAddressLimit - surface.gpuAddress) {
		return false;
	}

	// Restrict pitches/coordinates to positive signed-16-bit values. Only
	// 32-bit pixels, linear surfaces and origin-to-origin copies are modeled.
	if (width == 0 || height == 0 || width > 32767 || height > 32767
		|| surface.pitchBytes == 0 || surface.pitchBytes > 32767
		|| (surface.pitchBytes & 3) != 0
		|| uint64_t(width) * 4 > surface.pitchBytes) {
		return false;
	}

	uint64_t end = uint64_t(height - 1) * surface.pitchBytes
		+ uint64_t(width) * 4;
	return end <= surface.allocationBytes;
}

inline bool
PackLinearCopy32(const LinearSurface& destination, const LinearSurface& source,
	uint32_t width, uint32_t height, uint32_t* output, size_t capacityDwords)
{
	if (output == NULL || capacityDwords < kCopyDwords
		|| !ValidateSurface(destination, width, height)
		|| !ValidateSurface(source, width, height)) {
		return false;
	}

	// Reject all overlapping VA allocations, including same-surface copies.
	// This cannot detect physical aliases; a future memory manager must do so.
	if (source.gpuAddress < destination.gpuAddress + destination.allocationBytes
		&& destination.gpuAddress < source.gpuAddress + source.allocationBytes) {
		return false;
	}

	const uint32_t packet[kCopyDwords] = {
		(uint32_t(2) << 29) | (uint32_t(0x42) << 22) | 8,
		(uint32_t(3) << 24) | destination.pitchBytes,
		0,
		(height << 16) | width,
		uint32_t(destination.gpuAddress),
		uint32_t(destination.gpuAddress >> 32),
		0,
		source.pitchBytes,
		uint32_t(source.gpuAddress),
		uint32_t(source.gpuAddress >> 32)
	};
	for (size_t i = 0; i < kCopyDwords; i++)
		output[i] = packet[i];
	return true;
}

} // namespace Gen12Experiment

#endif
