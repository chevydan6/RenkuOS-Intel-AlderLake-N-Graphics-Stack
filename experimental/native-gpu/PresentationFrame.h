// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_PRESENTATION_FRAME_H
#define NATIVE_GPU_PRESENTATION_FRAME_H
#include "ExecutionReport.h"
#include <stddef.h>
#include <string.h>
namespace NativeGpu {
enum { OpaqueBgrx32=1 };
struct FrameDescription { uint32_t width,height,stride,format; };
// CPU-visible snapshot only. No GPU address or pending GPU destination is
// accepted here. The consumer uses B_RGB32 (opaque B,G,R,x on this x86 target).
inline bool CopyFrameRows(const FrameDescription& f,const void* source,size_t sourceBytes,
 void* destination,size_t destinationBytes,uint32_t destinationStride)
{
 uint64_t row=uint64_t(f.width)*4;
 if(f.format!=OpaqueBgrx32 || !f.width || !f.height || !source || !destination
  || row>f.stride || row>destinationStride)return false;
 uint64_t sourceEnd=uint64_t(f.height-1)*f.stride+row;
 uint64_t destinationEnd=uint64_t(f.height-1)*destinationStride+row;
 if(sourceEnd>sourceBytes || destinationEnd>destinationBytes)return false;
 for(uint32_t y=0;y<f.height;y++)
  memcpy((uint8_t*)destination+size_t(y)*destinationStride,
   (const uint8_t*)source+size_t(y)*f.stride,size_t(row));
 return true;
}
class CompletionGate {
 bool fFailed=false;
 uint32_t fSequence=0;
public:
 bool CanSubmit() const {return !fFailed;}
 uint32_t LastSequence() const {return fSequence;}
 void Fail(){fFailed=true;}
 bool Complete(const native_gpu_execution_report& r) {
  if(fFailed)return false;
  if(r.version!=1 || r.status!=0 || r.stage!=14 || !r.load_attempted
   || r.sequence<=fSequence || r.observed_sequence!=r.sequence
   || r.observed_nonce!=r.nonce || r.observed_complement!=~r.nonce
   || r.copy_words!=1024 || r.copy_matched!=1024 || !(r.mode&0x200)
   || r.error_register || r.fault_register || r.group!=0 || r.csb!=0){Fail();return false;}
  fSequence=r.sequence;return true;
 }
};
}
#endif
