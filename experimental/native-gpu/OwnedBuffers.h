// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_OWNED_BUFFERS_H
#define NATIVE_GPU_OWNED_BUFFERS_H
#include "BufferCopy.h"
#include <LinearCopy.h>
namespace NativeGpu {
inline bool ValidCopy(uint32_t width,uint32_t height,uint32_t sourcePitch,uint32_t destinationPitch)
{
 // Addresses are the owner's two distinct private PPGTT pages. Validation
 // uses wide arithmetic before any packet building, staging or publication.
 return Gen12Experiment::ValidateSurface({0x200000,kNativeBufferBytes,sourcePitch},width,height)
  && Gen12Experiment::ValidateSurface({0x201000,kNativeBufferBytes,destinationPitch},width,height);
}
// Caller serializes all methods including CPU byte access through its session
// mutex. Staging is CPU-only heap memory, NEVER mapped or published to the GPU.
// The resident owner copies it into pinned, exclusive GPU backing for each call.
struct OwnedBuffers {
 enum State { CpuOwned, GpuInFlight, Unknown };
 struct Buffer { bool created; State state; uint32_t words[kNativeBufferBytes/4]; } buffers[2];
 uint32_t Create(uint32_t bytes) {
  if(bytes!=kNativeBufferBytes)return 0;
  for(unsigned i=0;i<2;i++)if(!buffers[i].created){
   buffers[i].created=true;buffers[i].state=CpuOwned;
   for(auto& word:buffers[i].words)word=0;
   return i+1;
  }
  return 0;
 }
 bool Owned(uint32_t handle) const {return handle>=1 && handle<=2 && buffers[handle-1].created;}
 bool Range(uint32_t handle,uint32_t offset,uint32_t bytes) const {
  return Owned(handle) && buffers[handle-1].state==CpuOwned && bytes>0
   && offset<=kNativeBufferBytes && bytes<=kNativeBufferBytes-offset;
 }
 bool CanCopy(const native_gpu_copy& c) const {
  return c.source!=c.destination && Range(c.source,0,kNativeBufferBytes)
   && Range(c.destination,0,kNativeBufferBytes)
   && ValidCopy(c.width,c.height,c.source_pitch,c.destination_pitch);
 }
 bool Begin(const native_gpu_copy& c) {
  if(!CanCopy(c))return false;
  buffers[c.source-1].state=buffers[c.destination-1].state=GpuInFlight;
  return true;
 }
 void Finish(bool retired) {
  for(auto& b:buffers)if(b.state==GpuInFlight)b.state=retired?CpuOwned:Unknown;
 }
};
}
#endif
