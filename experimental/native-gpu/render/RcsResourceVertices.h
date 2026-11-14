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
 // Keep this distinct page beyond the 64x64 target and post-sync page.
 static constexpr uint32_t VertexGpu=0x40b000;
 static bool Build(uint32_t* const* pages,uint32_t* resource,const uint32_t* words) {
  if(!pages || !resource || (reinterpret_cast<uintptr_t>(resource)&4095)
   || !RcsUserVertices::Validate(words))return false;
  for(unsigned i=0;i<RcsDrawImage::PageCount;i++)if(resource==pages[i])return false;
  if(!Patch(pages,words,VertexGpu))return false;
  memset(resource,0,4096);
  memcpy(resource,words,12*sizeof(uint32_t));
  return true;
 }
 static bool BuildAt(uint32_t* const* pages,const uint32_t* words,uint64_t gpu) {
  if(gpu<0x410000 || gpu>=0x420000 || (gpu&4095))return false;
  if(!Patch(pages,words,uint32_t(gpu)))return false;
  // An unchanged fixed vertex page must not accidentally satisfy the image
  // check if the packet address fails to use the public resource.
  memset(pages[RcsDrawImage::Vertices],0,4096);
  return true;
 }
 // Use after a prebuilt 64x64 image; keep its viewport, surfaces, and target.
 static bool PatchAddressOnly(uint32_t* const* pages,uint64_t gpu) {
  if(!pages || gpu<0x410000 || gpu>=0x420000 || (gpu&4095))return false;
  if(!PatchAddress(pages,uint32_t(gpu)))return false;
  memset(pages[RcsDrawImage::Vertices],0,4096);
  return true;
 }
private:
 static bool Patch(uint32_t* const* pages,const uint32_t* words,uint32_t gpu) {
  if(!RcsUserVertices::Build(pages,words))return false;
  return PatchAddress(pages,gpu);
 }
 static bool PatchAddress(uint32_t* const* pages,uint32_t gpu) {
  uint32_t* batch=pages[RcsDrawImage::Batch];
  unsigned found=RcsDrawImage::BatchDwords;
  for(unsigned i=0;i+4<RcsDrawImage::BatchDwords;i++)
   if(batch[i]==0x78080003 && batch[i+1]==0x00064010
    && batch[i+2]==0x00403000 && batch[i+3]==0 && batch[i+4]==0x30){
    if(found!=RcsDrawImage::BatchDwords)return false;
    found=i+2;
   }
  if(found==RcsDrawImage::BatchDwords)return false;
  batch[found]=gpu;
  return true;
 }
};
}
#endif
