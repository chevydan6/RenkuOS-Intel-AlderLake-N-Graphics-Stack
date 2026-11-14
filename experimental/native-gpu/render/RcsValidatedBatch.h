// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_VALIDATED_BATCH_H
#define NATIVE_GPU_RCS_VALIDATED_BATCH_H
#include "RcsDrawImage.h"
namespace NativeGpu {
// First public command BO contract. The entire 4 KiB page must equal the
// owner-proven draw batch, except for one owner-validated vertex BO address.
// This is a safe stepping stone toward a general packet validator, not one.
class RcsValidatedBatch {
public:
 static bool PatchVertex(uint32_t* words,uint32_t gpu) {
  if(!words || !ValidAddress(gpu))return false;
  unsigned index=VertexIndex();if(index==RcsDrawImage::BatchDwords)return false;
  for(unsigned i=0;i<1024;i++)if(words[i]!=Expected(i))return false;
  words[index]=gpu;return true;
 }
 static bool Validate(const uint32_t* words,unsigned dwords,uint32_t gpu) {
  if(!words || dwords!=1024 || !ValidAddress(gpu))return false;
  unsigned index=VertexIndex();if(index==RcsDrawImage::BatchDwords)return false;
  for(unsigned i=0;i<1024;i++)
   if(words[i]!=(i==index?gpu:Expected(i)))return false;
  return true;
 }
private:
 static bool ValidAddress(uint32_t gpu)
 {return gpu>=0x410000 && gpu<0x420000 && !(gpu&4095);}
 static uint32_t Expected(unsigned index)
 {return index<RcsDrawImage::BatchDwords?RcsDrawData::Batch[index]:0;}
 static unsigned VertexIndex() {
  unsigned found=RcsDrawImage::BatchDwords;
  for(unsigned i=0;i+4<RcsDrawImage::BatchDwords;i++)
   if(RcsDrawData::Batch[i]==0x78080003 && RcsDrawData::Batch[i+1]==0x00064010
    && RcsDrawData::Batch[i+2]==0x00403000 && RcsDrawData::Batch[i+3]==0
    && RcsDrawData::Batch[i+4]==0x30){
    if(found!=RcsDrawImage::BatchDwords)return RcsDrawImage::BatchDwords;
    found=i+2;
   }
  return found;
 }
};
}
#endif
