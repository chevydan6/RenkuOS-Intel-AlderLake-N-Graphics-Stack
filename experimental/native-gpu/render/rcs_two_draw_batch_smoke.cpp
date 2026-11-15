// SPDX-License-Identifier: MIT
#include "RcsTwoDrawBatch.h"
#include "RcsPublicShader.h"
#include <assert.h>
#include <string.h>

int main() {
 alignas(4096) uint32_t batch[1024]={};
 memcpy(batch,NativeGpu::RcsDrawData::Batch,
  sizeof(NativeGpu::RcsDrawData::Batch));
 batch[294]=0x410000;
 assert(NativeGpu::RcsPublicShader::PatchBatchWithVertex(batch,1024,
  0x412000,0x410000));
 assert(NativeGpu::RcsTwoDrawBatch::AppendContiguous(batch,1024));
 const uint32_t expected[]={
  0x78080003,0x00064010,0x00410030,0,0x30,
  0x7b000005,4,3,0,1,0,0,
  0x7a000004,0x00101020,0,0,0,0,0x05000000};
 assert(!memcmp(batch+316,expected,sizeof(expected)));
 assert(batch[294]==0x410000 && batch[334]==0x05000000);
 for(unsigned i=335;i<1024;i++)assert(batch[i]==0);
 assert(!NativeGpu::RcsTwoDrawBatch::AppendContiguous(batch,1024));

 memset(batch,0,sizeof(batch));
 memcpy(batch,NativeGpu::RcsDrawData::Batch,
  sizeof(NativeGpu::RcsDrawData::Batch));
 batch[294]=0x410000;
 assert(NativeGpu::RcsPublicShader::PatchBatchWithVertex(batch,1024,
  0x412000,0x410000));
 batch[323]=1;
 assert(!NativeGpu::RcsTwoDrawBatch::AppendContiguous(batch,1024));
 batch[323]=0;batch[294]=0x44fff0;
 assert(!NativeGpu::RcsTwoDrawBatch::AppendContiguous(batch,1024));

 memset(batch,0,sizeof(batch));
 for(unsigned i=0;i<NativeGpu::RcsDrawImage::BatchDwords;i++)
  batch[i]=NativeGpu::RcsDrawImage64::BatchWord(i);
 batch[294]=0x410000;
 assert(NativeGpu::RcsPublicShader::PatchBatchWithVertex(batch,1024,
  0x412000,0x410000,true));
 assert(NativeGpu::RcsTwoDrawBatch::AppendContiguous(batch,1024));
 assert(!memcmp(batch+316,expected,sizeof(expected)));
 assert(batch[290]==0x003f003f && batch[334]==0x05000000);
 return 0;
}
