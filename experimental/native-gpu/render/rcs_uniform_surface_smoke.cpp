// SPDX-License-Identifier: MIT
#include "RcsUniformSurface.h"
#include "RcsPublicShader.h"
#include "RcsResourceVertices.h"
#include <assert.h>
#include <stdio.h>

int main()
{
 alignas(4096) uint32_t backing[NativeGpu::RcsDrawImage::PageCount][1024]={};
 uint32_t* pages[NativeGpu::RcsDrawImage::PageCount]={};
 for(unsigned i=0;i<NativeGpu::RcsDrawImage::PageCount;i++)pages[i]=backing[i];
 assert(NativeGpu::RcsDrawImage::Build(pages,0));
 constexpr uint64_t shader=0x411000,vertex=0x412000,ubo=0x413000;
 assert(NativeGpu::RcsResourceVertices::PatchAddressOnly(pages,vertex));
 assert(NativeGpu::RcsPublicShader::PatchBatchWithVertex(pages[0],1024,shader,vertex));
 assert(NativeGpu::RcsUniformSurface::Patch(pages,shader,ubo));
 assert(pages[0][241]==0x00080000 && pages[0][248]==0x110c0);
 assert(pages[2][0]==64 && pages[2][1]==128);
 assert(pages[2][32]==0x87fd4000 && pages[2][33]==0x06000000);
 assert(pages[2][34]==15 && pages[2][40]==ubo);
 for(unsigned i=1;i<16;i++)if(i!=8)assert(pages[2][32+i]==(i==1?0x06000000:
  i==2?15u:0u));
 assert(!NativeGpu::RcsUniformSurface::Patch(pages,shader,ubo));
 assert(NativeGpu::RcsDrawImage::Build(pages,0));
 assert(NativeGpu::RcsResourceVertices::PatchAddressOnly(pages,vertex));
 assert(NativeGpu::RcsPublicShader::PatchBatchWithVertex(pages[0],1024,shader,vertex));
 assert(!NativeGpu::RcsUniformSurface::Patch(pages,shader,ubo+4));
 assert(NativeGpu::RcsUniformSurface::Patch(pages,shader,ubo+16));
 assert(pages[2][40]==ubo+16);
 puts("N100 UBO surface packet: PASS (CPU only)");
}
