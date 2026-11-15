// SPDX-License-Identifier: MIT
#include "RcsResourceTarget.h"
#include <stdio.h>

int main() {
 alignas(4096) uint32_t storage[NativeGpu::RcsDrawImage64::PageCount][1024]={};
 uint32_t* pages[NativeGpu::RcsDrawImage64::PageCount]={};
 for(unsigned i=0;i<NativeGpu::RcsDrawImage64::PageCount;i++)pages[i]=storage[i];
 if(!NativeGpu::RcsDrawImage::Build(pages,0)
  || !NativeGpu::RcsResourceTarget::Patch(pages,0x411000,32)
  || pages[NativeGpu::RcsDrawImage::Surface][24]!=0x411000)return 1;
 if(!NativeGpu::RcsDrawImage64::Build(pages,0)
  || !NativeGpu::RcsResourceTarget::Patch(pages,0x41c000,64)
  || pages[NativeGpu::RcsDrawImage::Surface][24]!=0x41c000)return 2;
 if(!NativeGpu::RcsDrawImage64::Build(pages,0)
  || !NativeGpu::RcsResourceTarget::Patch(pages,0x44c000,64)
  || NativeGpu::RcsResourceTarget::Patch(pages,0x44d000,64)
  || NativeGpu::RcsResourceTarget::Patch(pages,0x411001,32)
  || NativeGpu::RcsResourceTarget::Patch(pages,0x406000,32))return 3;
 puts("N100 direct resource target surface patch: PASS (CPU only)");
 return 0;
}
