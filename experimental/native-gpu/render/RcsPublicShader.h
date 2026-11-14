// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_PUBLIC_SHADER_H
#define NATIVE_GPU_RCS_PUBLIC_SHADER_H
#include "RcsDrawImage.h"
namespace NativeGpu {
// Bounded first shader BO contract. Only the proven FS code is accepted; its
// owner-selected VA replaces two PS pointers and widens the instruction
// window to cover the prebound resource arena. No arbitrary GPU code runs.
class RcsPublicShader {
public:
 static bool ValidateFragment(const uint32_t* words,unsigned dwords) {
  if(!words || dwords!=1024)return false;
  for(unsigned i=0;i<1024;i++)
   if(words[i]!=(i<kFsDwords?RcsDrawData::Fs[i]:0))return false;
  return true;
 }
 static bool PatchBatch(uint32_t* words,unsigned dwords,uint64_t gpu) {
  if(!words || dwords!=1024 || !ValidGpu(gpu))return false;
  for(unsigned i=0;i<1024;i++)if(words[i]!=Original(i))return false;
  words[44]=0x20001; // STATE_BASE_ADDRESS Instruction Buffer Size: 32 pages.
  words[239]=uint32_t(gpu-RcsDrawImage::GpuBase); // PS SIMD8 KSP.
  words[248]=words[239]+0x80; // PS SIMD16 KSP.
  return true;
 }
 static bool PatchBatchWithVertex(uint32_t* words,unsigned dwords,
  uint64_t shaderGpu,uint64_t vertexGpu) {
  if(!words || dwords!=1024 || !ValidGpu(shaderGpu) || !ValidGpu(vertexGpu)
   || shaderGpu==vertexGpu)return false;
  unsigned slot=1024;
  for(unsigned i=2;i+2<RcsDrawImage::BatchDwords;i++)
   if(Original(i-2)==0x78080003 && Original(i-1)==0x00064010
    && Original(i)==0x00403000 && Original(i+1)==0 && Original(i+2)==0x30){
    if(slot!=1024)return false;
    slot=i;
   }
  if(slot==1024)return false;
  for(unsigned i=0;i<1024;i++)
   if(words[i]!=(i==slot?uint32_t(vertexGpu):Original(i)))return false;
  words[44]=0x20001;
  words[239]=uint32_t(shaderGpu-RcsDrawImage::GpuBase);
  words[248]=words[239]+0x80;
  return true;
 }
 static bool ValidateBatch(const uint32_t* words,unsigned dwords,uint64_t gpu) {
  if(!words || dwords!=1024 || !ValidGpu(gpu))return false;
  uint32_t offset=uint32_t(gpu-RcsDrawImage::GpuBase);
  for(unsigned i=0;i<1024;i++){
   uint32_t expected=i==44?0x20001:i==239?offset:i==248?offset+0x80:Original(i);
   if(words[i]!=expected)return false;
  }
  return true;
 }
private:
 static constexpr unsigned kFsDwords=sizeof(RcsDrawData::Fs)/sizeof(uint32_t);
 static_assert(sizeof(RcsDrawData::Fs)==208,"Mesa 26.3.0 FS profile changed");
 static_assert(RcsDrawData::Batch[44]==0x8001 && RcsDrawData::Batch[239]==0x5000
  && RcsDrawData::Batch[248]==0x5080,"Mesa 26.3.0 batch layout changed");
 static bool ValidGpu(uint64_t gpu)
 {return gpu>=0x410000 && gpu<0x420000 && !(gpu&4095);}
 static uint32_t Original(unsigned index)
 {return index<RcsDrawImage::BatchDwords?RcsDrawData::Batch[index]:0;}
};
}
#endif
