// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_RESOURCE_TARGET_H
#define NATIVE_GPU_RCS_RESOURCE_TARGET_H
#include "RcsDrawImage64.h"
#include "RcsResourceRange.h"
#include <stdint.h>

namespace NativeGpu {
// Redirect the fixed linear BGRA render surface to an owner-bound resource.
// The caller resolves the BO and owns visibility and lifetime checks.
class RcsResourceTarget {
public:
 static bool Patch(uint32_t* const* pages,uint64_t gpu,unsigned width) {
  if(!pages || !pages[RcsDrawImage::Surface] || (width!=32 && width!=64)
   || gpu<RcsResourceRange::Base || (gpu&4095)
   || gpu>RcsResourceRange::End-(width==64?16384u:4096u))return false;
  uint32_t* surface=pages[RcsDrawImage::Surface];
  if(surface[18]!=(width==64?0x003f003f:0x001f001f)
   || surface[19]!=(width==64?0x000000ff:0x0000007f)
   || surface[24]!=0x00406000 || surface[25])return false;
  surface[24]=uint32_t(gpu);
  return true;
 }
};
}
#endif
