// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RESOURCE_COMMAND_H
#define NATIVE_GPU_RESOURCE_COMMAND_H
#include <stdint.h>
#define NATIVE_GPU_RESOURCE_IOCTL UINT32_C(0x91504e31)
enum native_gpu_resource_action : uint32_t {
 NATIVE_GPU_RESOURCE_CREATE=1, NATIVE_GPU_RESOURCE_WRITE=2,
 NATIVE_GPU_RESOURCE_READ=3, NATIVE_GPU_RESOURCE_DESTROY=4
};
// One bounded transfer per ioctl. A successful CREATE returns a file-local
// handle, rounded size and owner-selected GPU VA; users cannot choose a VA.
struct native_gpu_resource_command {
 uint32_t version,action,handle,bytes;
 uint64_t size,offset,gpu;
 uint8_t data[4096];
};
static_assert(sizeof(native_gpu_resource_command)==4136,"resource ioctl ABI");
inline bool NativeGpuResourceValid(const native_gpu_resource_command& c) {
 if(c.version!=1)return false;
 switch(c.action){
 case NATIVE_GPU_RESOURCE_CREATE:
  return c.handle==0 && c.bytes==0 && c.offset==0 && c.gpu==0 && c.size>0;
 case NATIVE_GPU_RESOURCE_WRITE:
 case NATIVE_GPU_RESOURCE_READ:
  return c.handle!=0 && c.bytes>0 && c.bytes<=sizeof(c.data)
   && c.size==0 && c.gpu==0;
 case NATIVE_GPU_RESOURCE_DESTROY:
  return c.handle!=0 && c.bytes==0 && c.size==0 && c.offset==0 && c.gpu==0;
 default:return false;
 }
}
#endif
