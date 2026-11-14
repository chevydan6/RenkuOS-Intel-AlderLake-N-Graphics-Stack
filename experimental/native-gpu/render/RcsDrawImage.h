// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chevydan6
#ifndef NATIVE_GPU_RCS_DRAW_IMAGE_H
#define NATIVE_GPU_RCS_DRAW_IMAGE_H
#include <stdint.h>
#include "draw/RcsDrawGenerated.h"
namespace NativeGpu {
// Fixed kernel-owned workload. CPU construction is not hardware draw proof.
class RcsDrawImage {
public:
 static constexpr uint64_t GpuBase=0x400000;
 static constexpr unsigned PageCount=8, PageBytes=4096;
 static constexpr unsigned BatchDwords=sizeof(RcsDrawData::Batch)/sizeof(uint32_t);
 static constexpr uint32_t Poison=0x19537bad, RenderedPixel=0xffff4000;
 static constexpr unsigned TargetWidth=32, TargetHeight=32, TargetPitch=128;
 enum Page { Batch=0, Dynamic=1, Surface=2, Vertices=3, Vs=4, Fs=5, Target=6, Reserved=7 };
 static bool Build(uint32_t* const* pages, unsigned variant, uint64_t gpuBase=GpuBase) {
  if (!pages || variant>=4 || gpuBase!=GpuBase) return false;
  // Require whole, distinct CPU page mappings; validate all before writing any.
  for(unsigned i=0;i<PageCount;++i) {
   if (!pages[i] || (reinterpret_cast<uintptr_t>(pages[i])&(PageBytes-1))) return false;
   for(unsigned j=0;j<i;++j) if(pages[j]==pages[i]) return false;
  }
  for(unsigned p=0;p<PageCount;++p)
   for(unsigned i=0;i<PageBytes/4;++i) pages[p][i]=0;
  Copy(pages[Batch],RcsDrawData::Batch,BatchDwords);
  Copy(pages[Dynamic],RcsDrawData::Dynamic,sizeof(RcsDrawData::Dynamic)/4);
  Copy(pages[Surface],RcsDrawData::Surface,sizeof(RcsDrawData::Surface)/4);
  Copy(pages[Vs],RcsDrawData::Vs,sizeof(RcsDrawData::Vs)/4);
  Copy(pages[Fs],RcsDrawData::Fs,sizeof(RcsDrawData::Fs)/4);
  // NDC x/y at +/-0.75, z=0.5, w=1. Viewport maps to x/y=4..28.
  static constexpr uint32_t corners[4][2]={{0xbf400000,0xbf400000},{0x3f400000,0xbf400000},
   {0x3f400000,0x3f400000},{0xbf400000,0x3f400000}};
  for(unsigned i=0;i<3;++i) {
   const unsigned c=(variant+i)&3;
   pages[Vertices][i*4]=corners[c][0]; pages[Vertices][i*4+1]=corners[c][1];
   pages[Vertices][i*4+2]=0x3f000000; pages[Vertices][i*4+3]=0x3f800000;
  }
  for(unsigned i=0;i<PageBytes/4;++i) pages[Target][i]=Poison;
  return true;
 }
 // Safely inside each triangle and its complementary half; avoid raster edges.
 static unsigned InteriorX(unsigned v) { return v<2?20:10; }
 static unsigned InteriorY(unsigned v) { return (v==0 || v==3)?10:20; }
 static unsigned ExteriorX(unsigned v) { return 31-InteriorX(v); }
 static unsigned ExteriorY(unsigned v) { return 31-InteriorY(v); }
private:
 static void Copy(uint32_t* dst,const uint32_t* src,unsigned count) {
  for(unsigned i=0;i<count;++i) dst[i]=src[i];
 }
};
}
#endif
