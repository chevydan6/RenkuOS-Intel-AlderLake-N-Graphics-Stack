// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_DRAW_IMAGE64_H
#define NATIVE_GPU_RCS_DRAW_IMAGE64_H
#include "RcsDrawImage.h"
#include <stdint.h>
#include <string.h>

namespace NativeGpu {
// Build the isolated 64x64 packet image from the proven 32x32 image and the
// exact Mesa 26.3 delta. No hardware submission or ownership is implied.
class RcsDrawImage64 {
public:
 static constexpr unsigned PageCount=11, TargetPages=4, TargetWidth=64;
 static constexpr unsigned TargetHeight=64, TargetPitch=256;
 static constexpr unsigned Target=6, PostSync=10;

 static uint32_t BatchWord(unsigned index) {
  switch(index){
   case 2:case 25:return 0x0040a000;
   case 41:case 44:return 0x0000b001;
   case 53:return 0x0040a008;
   case 290:return 0x003f003f;
   default:return index<RcsDrawImage::BatchDwords?RcsDrawData::Batch[index]:0;
  }
 }

 static bool Build(uint32_t* const* pages,unsigned variant,
  uint64_t gpuBase=RcsDrawImage::GpuBase)
 {
  if(!pages || variant>=4 || gpuBase!=RcsDrawImage::GpuBase)return false;
  for(unsigned i=0;i<PageCount;i++){
   if(!pages[i] || (reinterpret_cast<uintptr_t>(pages[i])&4095))return false;
   for(unsigned j=0;j<i;j++)if(pages[i]==pages[j])return false;
  }
  if(!RcsDrawImage::Build(pages,variant,gpuBase))return false;
  static constexpr Patch batch[]={
   {2,0x0040a000},{25,0x0040a000},{41,0x0000b001},
   {44,0x0000b001},{53,0x0040a008},{290,0x003f003f}};
  static constexpr Patch dynamic[]={
   {0,0x42000000},{1,0x42000000},{3,0x42000000},
   {4,0x42000000},{13,0x427c0000},{15,0x427c0000},
   {65,0x003f003f}};
  static constexpr Patch surface[]={{18,0x003f003f},{19,0x000000ff}};
  Apply(pages[RcsDrawImage::Batch],batch);
  Apply(pages[RcsDrawImage::Dynamic],dynamic);
  Apply(pages[RcsDrawImage::Surface],surface);
  for(unsigned p=Target;p<Target+TargetPages;p++)
   for(unsigned i=0;i<1024;i++)pages[p][i]=RcsDrawImage::Poison;
  memset(pages[PostSync],0,4096);
  return true;
 }
private:
 struct Patch {unsigned index;uint32_t word;};
 template<size_t N> static void Apply(uint32_t* words,const Patch (&patches)[N])
 {for(const auto& patch:patches)words[patch.index]=patch.word;}
};
}
#endif
