// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_TWO_DRAW_BATCH_H
#define NATIVE_GPU_RCS_TWO_DRAW_BATCH_H
#include "RcsResourceRange.h"
#include <string.h>

namespace NativeGpu {
// Insert one contiguous three-vertex draw before the final completion packet.
// The owner has already patched and validated the single-draw batch.
class RcsTwoDrawBatch {
public:
 static bool AppendContiguous(uint32_t* batch,unsigned dwords) {
  if(!batch || dwords!=1024 || batch[292]!=0x78080003
   || batch[293]!=0x00064010 || batch[295]!=0 || batch[296]!=0x30
   || batch[309]!=0x7b000005 || batch[310]!=4 || batch[311]!=3
   || batch[312]!=0 || batch[313]!=1 || batch[314]!=0 || batch[315]!=0
   || batch[316]!=0x7a000004 || batch[322]!=0x05000000)return false;
  uint32_t first=batch[294];
  if(first<RcsResourceRange::Base || first>RcsResourceRange::End-96
   || (first&15) || (first&4095)>4096-96)return false;
  for(unsigned i=323;i<335;i++)if(batch[i])return false;
  memmove(batch+328,batch+316,7*sizeof(uint32_t));
  memcpy(batch+316,batch+292,5*sizeof(uint32_t));
  memcpy(batch+321,batch+309,7*sizeof(uint32_t));
  batch[318]=first+48;
  return true;
 }
};
static_assert(RcsDrawData::Batch[292]==0x78080003
 && RcsDrawData::Batch[294]==0x403000
 && RcsDrawData::Batch[309]==0x7b000005
 && RcsDrawData::Batch[316]==0x7a000004
 && RcsDrawData::Batch[322]==0x05000000,
 "Mesa 26.3.0 N100 batch packet locations changed");
}
#endif
