// SPDX-License-Identifier: MIT
#include "ResourceCommand.h"
#include <assert.h>
#include <stdio.h>
int main() {
 native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_CREATE;c.size=1;
 assert(NativeGpuResourceValid(c));
 c.gpu=0x410000;assert(!NativeGpuResourceValid(c));c.gpu=0;
 c.action=NATIVE_GPU_RESOURCE_WRITE;c.size=0;c.handle=1;c.bytes=48;
 assert(NativeGpuResourceValid(c));
 c.bytes=4097;assert(!NativeGpuResourceValid(c));c.bytes=48;
 c.handle=0;assert(!NativeGpuResourceValid(c));c.handle=1;
 c.action=NATIVE_GPU_RESOURCE_READ;assert(NativeGpuResourceValid(c));
 c.action=NATIVE_GPU_RESOURCE_DESTROY;c.bytes=0;assert(NativeGpuResourceValid(c));
 c.offset=1;assert(!NativeGpuResourceValid(c));c.offset=0;
 c.action=NATIVE_GPU_RESOURCE_COPY;c.gpu=2;c.bytes=48;
 assert(NativeGpuResourceValid(c));
 c.gpu=UINT64_MAX;assert(!NativeGpuResourceValid(c));c.gpu=2;
 c.bytes=64*4096+1;assert(!NativeGpuResourceValid(c));c.bytes=48;
 c.action=999;assert(!NativeGpuResourceValid(c));
 c.version=2;assert(!NativeGpuResourceValid(c));
 puts("Native GPU resource ioctl validation: PASS (CPU only)");
}
