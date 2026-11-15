// SPDX-License-Identifier: MIT
#include "RcsIndexedBatch.h"
#include <assert.h>
#include <string.h>

int main() {
 uint32_t batch[1024]={};
 memcpy(batch,NativeGpu::RcsDrawData::Batch,
  sizeof(NativeGpu::RcsDrawData::Batch));
 assert(!NativeGpu::RcsIndexedBatch::Patch(batch,1023));
 batch[294]=NativeGpu::RcsResourceRange::Base;
 assert(NativeGpu::RcsIndexedBatch::Patch(batch,1024));
 assert(batch[296]==64 && batch[309]==0x780a0003
  && batch[310]==0x904 && batch[311]==NativeGpu::RcsIndexedBatch::IndexGpu
  && batch[312]==0 && batch[313]==6);
 assert(batch[314]==0x7b000808 && batch[315]==0x104
  && batch[316]==3 && batch[321]==0 && batch[322]==0
  && batch[323]==0 && batch[324]==0x7a000004
  && batch[330]==0x05000000);
 assert(!NativeGpu::RcsIndexedBatch::Patch(batch,1024));
 memset(batch,0,sizeof(batch));
 memcpy(batch,NativeGpu::RcsDrawData::Batch,
  sizeof(NativeGpu::RcsDrawData::Batch));
 batch[294]=NativeGpu::RcsResourceRange::Base;
 assert(!NativeGpu::RcsIndexedBatch::Patch(batch,1024,5));
 assert(NativeGpu::RcsIndexedBatch::Patch(batch,1024,6));
 assert(batch[313]==12 && batch[316]==6
  && batch[314]==0x7b000808 && batch[315]==0x104);
}
