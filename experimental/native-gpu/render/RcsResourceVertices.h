// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_RESOURCE_VERTICES_H
#define NATIVE_GPU_RCS_RESOURCE_VERTICES_H
#include "RcsUserVertices.h"
#include <string.h>
namespace NativeGpu {
// First distinct GPU resource: one owner-allocated page at a fixed PPGTT VA.
// Only the vertex-buffer packet's low address changes from the v2 workload.
class RcsResourceVertices {
public:
 static constexpr uint32_t VertexGpu=0x408000;
 static bool Build(uint32_t* const* pages,uint32_t* resource,const uint32_t* words) {
  if(!pages || !resource || (reinterpret_cast<uintptr_t>(resource)&4095)
   || !RcsUserVertices::Validate(words))return false;
  for(unsigned i=0;i<RcsDrawImage::PageCount;i++)if(resource==pages[i])return false;
  if(!RcsUserVertices::Build(pages,words))return false;
  uint32_t* batch=pages[RcsDrawImage::Batch];
  unsigned found=RcsDrawImage::BatchDwords;
  for(unsigned i=0;i+4<RcsDrawImage::BatchDwords;i++)
   if(batch[i]==0x78080003 && batch[i+1]==0x00064010
    && batch[i+2]==0x00403000 && batch[i+3]==0 && batch[i+4]==0x30){
    if(found!=RcsDrawImage::BatchDwords)return false;
    found=i+2;
   }
  if(found==RcsDrawImage::BatchDwords)return false;
  memset(resource,0,4096);
  memcpy(resource,words,12*sizeof(uint32_t));
  batch[found]=VertexGpu;
  return true;
 }
};
}
#endif
