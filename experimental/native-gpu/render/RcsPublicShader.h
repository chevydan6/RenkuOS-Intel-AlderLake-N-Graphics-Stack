// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_PUBLIC_SHADER_H
#define NATIVE_GPU_RCS_PUBLIC_SHADER_H
#include "RcsDrawImage.h"
namespace NativeGpu {
// Bounded shader BO contract. Accept the proven 208-byte FS instruction template
// with two equal green immediates below 1.0 and zero padding. The owner-selected
// VA replaces two PS pointers; other GPU instructions remain rejected.
class RcsPublicShader {
public:
 static bool ValidateFragment(const uint32_t* words,unsigned dwords) {
  if(!words || dwords!=1024)return false;
  const uint32_t green=words[7];
  if(green>=0x3f800000)return false; // Reject negative, 1.0, larger, NaN, Inf.
  for(unsigned i=0;i<1024;i++)
   if(words[i]!=(i==7 || i==39?green:
    i<kFsDwords?RcsDrawData::Fs[i]:0))return false;
  return true;
 }
 static bool ExpectedPixel(const uint32_t* words,unsigned dwords,uint32_t& pixel) {
  if(!ValidateFragment(words,dwords))return false;
  const uint32_t green=words[7],exponent=green>>23;
  uint32_t channel=0;
  if(exponent>86){
   const uint32_t mantissa=(green&0x7fffff)|0x800000;
   const unsigned shift=150-exponent;
   channel=uint32_t((uint64_t(mantissa)*255+(uint64_t(1)<<(shift-1)))>>shift);
  }
  pixel=0xffff0000|(channel<<8); // BGRA8 UNORM, round to nearest.
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
 static_assert(RcsDrawData::Fs[7]==0x3e800000 && RcsDrawData::Fs[39]==0x3e800000,
  "fragment green immediate offsets changed");
 static_assert(RcsDrawData::Batch[44]==0x8001 && RcsDrawData::Batch[239]==0x5000
  && RcsDrawData::Batch[248]==0x5080,"Mesa 26.3.0 batch layout changed");
 static bool ValidGpu(uint64_t gpu)
 {return gpu>=0x410000 && gpu<0x420000 && !(gpu&4095);}
 static uint32_t Original(unsigned index)
 {return index<RcsDrawImage::BatchDwords?RcsDrawData::Batch[index]:0;}
};
}
#endif
