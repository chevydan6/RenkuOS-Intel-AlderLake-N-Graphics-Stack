// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_RESOURCE_RANGE_H
#define NATIVE_GPU_RCS_RESOURCE_RANGE_H
#include "RcsDrawImage.h"
namespace NativeGpu {
// Prebound before the first RCS publication; no live PPGTT leaf changes.
struct RcsResourceRange {
 static constexpr uint64_t Base=0x410000;
 static constexpr unsigned Pages=64;
 static constexpr uint64_t End=Base+uint64_t(Pages)*4096;
 // STATE_BASE_ADDRESS instruction window begins at the private 0x400000 page.
 static constexpr uint32_t InstructionBufferSize=uint32_t(End-RcsDrawImage::GpuBase)|1;
};
static_assert(RcsResourceRange::End<=RcsDrawImage::GpuBase+0x200000,
 "resource arena must stay inside the prebuilt PPGTT window");
static_assert(RcsResourceRange::InstructionBufferSize==0x50001,
 "instruction window packet encoding changed");
}
#endif
