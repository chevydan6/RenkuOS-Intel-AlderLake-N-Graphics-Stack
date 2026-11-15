// SPDX-License-Identifier: MIT
#include "RcsUniformShader.h"
#include <assert.h>
#include <stdio.h>

int main()
{
 uint32_t page[1024]={};
 for(unsigned i=0;i<84;i++)page[i]=NativeGpu::RcsUboFragment::Words[i];
 assert(NativeGpu::RcsUniformShader::ValidatePage(page,1024));
 page[84]=1;assert(!NativeGpu::RcsUniformShader::ValidatePage(page,1024));
 page[84]=0;page[0]^=1;assert(!NativeGpu::RcsUniformShader::ValidatePage(page,1024));
 const uint32_t a[4]={0x3f800000,0x3e800000,0,0x3f800000};
 const uint32_t b[4]={0,0x3f800000,0x3f000000,0x3f800000};
 uint32_t pixel=0;
 assert(NativeGpu::RcsUniformShader::ExpectedPixel(a,pixel) && pixel==0xffff4000);
 assert(NativeGpu::RcsUniformShader::ExpectedPixel(b,pixel) && pixel==0xff00ff80);
 const uint32_t invalid[4]={0x3f800001,0,0,0x3f800000};
 assert(!NativeGpu::RcsUniformShader::ExpectedPixel(invalid,pixel));
 puts("N100 exact UBO FS and pixel oracle: PASS (CPU only)");
}
