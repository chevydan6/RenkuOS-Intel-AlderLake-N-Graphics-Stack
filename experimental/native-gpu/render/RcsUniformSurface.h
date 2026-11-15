// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_UNIFORM_SURFACE_H
#define NATIVE_GPU_RCS_UNIFORM_SURFACE_H
#include "RcsDrawImage.h"
#include "RcsResourceRange.h"

namespace NativeGpu {
// Add one RAW 16-byte UBO at BTI 1. BTI 0 remains the render target.
// Words come from Mesa 26.3 gen120_pack using ISL's Gen12 buffer fields.
class RcsUniformSurface {
public:
 static bool Patch(uint32_t* const* pages,uint64_t shaderGpu,uint64_t uboGpu) {
  if(!pages || !pages[RcsDrawImage::Batch] || !pages[RcsDrawImage::Surface]
   || shaderGpu<RcsResourceRange::Base || shaderGpu>=RcsResourceRange::End
   || uboGpu<RcsResourceRange::Base || uboGpu>=RcsResourceRange::End
   || (shaderGpu&4095) || (uboGpu&4095) || shaderGpu==uboGpu)return false;
  uint32_t* batch=pages[RcsDrawImage::Batch];
  uint32_t* surface=pages[RcsDrawImage::Surface];
  if(batch[238]!=0x7820000a || batch[241]!=0x00040000
   || batch[248]!=uint32_t(shaderGpu-RcsDrawImage::GpuBase)+0x80
   || surface[0]!=64 || surface[1]!=0)return false;
  for(unsigned i=32;i<48;i++)if(surface[i])return false;
  batch[241]=0x00080000; // 3DSTATE_PS.BindingTableEntryCount = 2.
  batch[248]+=0x40; // SIMD16 program begins at +192 bytes.
  surface[1]=128; // BTI 1 points to the second 64-byte surface state.
  for(unsigned i=0;i<16;i++)surface[32+i]=kRaw16[i];
  surface[40]=uint32_t(uboGpu);
  return true;
 }
private:
 static constexpr uint32_t kRaw16[16]={
  0x87fd4000,0x06000000,0x0000000f,0,0,0,0,0,
  0x00410000,0,0,0,0,0,0,0
 };
};
}
#endif
