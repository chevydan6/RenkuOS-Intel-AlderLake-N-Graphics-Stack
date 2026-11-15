// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_INDEXED_BATCH_H
#define NATIVE_GPU_RCS_INDEXED_BATCH_H
#include "RcsResourceRange.h"
#include <string.h>

namespace NativeGpu {
// Bounded N100 indexed triangle lists. The owner stages the indices in its
// private vertex page; the public vertex BO feeds the GPU.
class RcsIndexedBatch {
public:
 static constexpr uint32_t IndexGpu=RcsDrawImage::GpuBase
  +RcsDrawImage::Vertices*4096+256;
 static bool Patch(uint32_t* batch,unsigned dwords,unsigned indexCount=3,
  unsigned vertexBytes=64) {
  if(!batch || dwords!=1024 || indexCount<3 || indexCount>255
   || indexCount%3 || vertexBytes<48 || vertexBytes>4096
   || vertexBytes%16
   || batch[292]!=0x78080003
   || batch[293]!=0x00064010 || batch[296]!=0x30
   || batch[309]!=0x7b000005 || batch[310]!=4 || batch[311]!=3
   || batch[312]!=0 || batch[313]!=1 || batch[314]!=0
   || batch[315]!=0 || batch[316]!=0x7a000004
   || batch[322]!=0x05000000)return false;
  if(batch[294]<RcsResourceRange::Base
   || batch[294]>RcsResourceRange::End-vertexBytes
   || (batch[294]&15) || (batch[294]&4095)>4096-vertexBytes)return false;
  for(unsigned i=323;i<331;i++)if(batch[i])return false;
  memmove(batch+324,batch+316,7*sizeof(uint32_t));
  memmove(batch+314,batch+309,7*sizeof(uint32_t));
  memset(batch+321,0,3*sizeof(uint32_t));
  batch[296]=vertexBytes;
  batch[309]=0x780a0003; // 3DSTATE_INDEX_BUFFER, 5 dwords.
  batch[310]=0x00000904; // WORD indices, MOCS 4, L3 bypass disabled.
  batch[311]=IndexGpu;
  batch[312]=0;
  batch[313]=indexCount*2;
  batch[314]=0x7b000808; // 10-dword 3DPRIMITIVE with extended parameters.
  batch[315]|=0x100;     // Vertex Access Type = RANDOM.
  batch[316]=indexCount; // TRILIST vertices per instance.
  return true;
 }
};
static_assert(RcsDrawData::Batch[292]==0x78080003
 && RcsDrawData::Batch[296]==0x30
 && RcsDrawData::Batch[309]==0x7b000005
 && RcsDrawData::Batch[316]==0x7a000004
 && RcsDrawData::Batch[322]==0x05000000,
 "Mesa 26.3.0 N100 indexed packet insertion locations changed");
}
#endif
