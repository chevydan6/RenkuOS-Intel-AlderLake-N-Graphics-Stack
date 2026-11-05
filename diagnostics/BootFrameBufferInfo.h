// SPDX-License-Identifier: MIT
#ifndef GPU_BOOT_FRAME_BUFFER_INFO_H
#define GPU_BOOT_FRAME_BUFFER_INFO_H

#include <stdint.h>

// Diagnostic ABI v1: no pointers or architecture-dependent fields.
// This describes the firmware boot snapshot, never the current display state.
struct BootFrameBufferInfo {
	uint32_t version;
	uint32_t size;
	uint64_t physicalBase;
	uint64_t visibleBytes;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t bytesPerRow;
	uint32_t flags;
	uint32_t reserved;
};

enum {
	kBootFrameBufferSnapshot = 1
};

static_assert(sizeof(BootFrameBufferInfo) == 48, "boot inventory ABI size");

static inline bool
BuildBootFrameBufferInfo(uint64_t physicalBase, int32_t width, int32_t height,
	int32_t depth, int32_t bytesPerRow, BootFrameBufferInfo& output)
{
	if (width <= 0 || height <= 0 || depth <= 0 || depth > 32 || bytesPerRow <= 0)
		return false;
	uint64_t minimumStride = (uint64_t(width) * depth + 7) / 8;
	if (uint64_t(bytesPerRow) < minimumStride)
		return false;
	uint64_t visibleBytes = uint64_t(bytesPerRow) * height;
	if (physicalBase > UINT64_MAX - visibleBytes)
		return false;

	BootFrameBufferInfo info = {};
	info.version = 1;
	info.size = sizeof(info);
	info.physicalBase = physicalBase;
	info.visibleBytes = visibleBytes;
	info.width = width;
	info.height = height;
	info.depth = depth;
	info.bytesPerRow = bytesPerRow;
	info.flags = kBootFrameBufferSnapshot;
	output = info;
	return true;
}

#endif
