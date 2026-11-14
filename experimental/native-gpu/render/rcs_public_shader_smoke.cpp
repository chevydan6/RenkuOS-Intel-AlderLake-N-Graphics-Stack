// SPDX-License-Identifier: MIT
#include "RcsPublicShader.h"
#include "RcsResourceVertices.h"
#include <stdio.h>
#include <string.h>

int main() {
 alignas(4096) uint32_t batch[1024]={},shader[1024]={};
 memcpy(batch,NativeGpu::RcsDrawData::Batch,sizeof(NativeGpu::RcsDrawData::Batch));
 memcpy(shader,NativeGpu::RcsDrawData::Fs,sizeof(NativeGpu::RcsDrawData::Fs));
 if(!NativeGpu::RcsPublicShader::ValidateFragment(shader,1024))return 1;
 uint32_t pixel=0;
 if(!NativeGpu::RcsPublicShader::ExpectedPixel(shader,1024,pixel)
  || pixel!=NativeGpu::RcsDrawImage::RenderedPixel)return 16;
 if(!NativeGpu::RcsPublicShader::PatchBatch(batch,1024,0x410000))return 2;
 if(batch[44]!=0x20001 || batch[239]!=0x10000 || batch[248]!=0x10080)return 3;
 if(!NativeGpu::RcsPublicShader::ValidateBatch(batch,1024,0x410000))return 4;
 batch[0]^=1;
 if(NativeGpu::RcsPublicShader::ValidateBatch(batch,1024,0x410000))return 5;
 batch[0]^=1;
 shader[0]^=1;
 if(NativeGpu::RcsPublicShader::ValidateFragment(shader,1024))return 6;
 shader[0]^=1;
 shader[1023]=1;
 if(NativeGpu::RcsPublicShader::ValidateFragment(shader,1024))return 7;
 shader[1023]=0;
 shader[7]=0x3f000000;shader[39]=0x3f000000;
 if(!NativeGpu::RcsPublicShader::ValidateFragment(shader,1024))return 14;
 if(!NativeGpu::RcsPublicShader::ExpectedPixel(shader,1024,pixel)
  || pixel!=0xffff8000)return 17;
 shader[39]=0x3e800000;
 if(NativeGpu::RcsPublicShader::ValidateFragment(shader,1024))return 15;
 if(NativeGpu::RcsPublicShader::ExpectedPixel(shader,1024,pixel))return 18;
 struct ColorCase {uint32_t bits,pixel;};
 const ColorCase accepted[]={
  {0x00000000,0xffff0000},{0x3dcccccd,0xffff1a00},
  {0x3e800000,0xffff4000},{0x3f000000,0xffff8000},
  {0x3f400000,0xffffbf00},{0x3f666666,0xffffe500}};
 for(const auto& color:accepted){
  shader[7]=shader[39]=color.bits;
  if(!NativeGpu::RcsPublicShader::ValidateFragment(shader,1024)
   || !NativeGpu::RcsPublicShader::ExpectedPixel(shader,1024,pixel)
   || pixel!=color.pixel)return 19;
 }
 const uint32_t rejected[]={0x3f800000,0x3f800001,0xbf000000,0x7f800000,0x7fc00000};
 for(uint32_t bits:rejected){
  shader[7]=shader[39]=bits;
  if(NativeGpu::RcsPublicShader::ValidateFragment(shader,1024)
   || NativeGpu::RcsPublicShader::ExpectedPixel(shader,1024,pixel))return 20;
 }
 shader[7]=shader[39]=0x3e800000;
 uint32_t original[1024]={};
 memcpy(original,NativeGpu::RcsDrawData::Batch,sizeof(NativeGpu::RcsDrawData::Batch));
 uint32_t invalid[1024];memcpy(invalid,original,sizeof(invalid));
 if(NativeGpu::RcsPublicShader::PatchBatch(invalid,1024,0x420000)
  || memcmp(invalid,original,sizeof(invalid)))return 8;
 invalid[1]^=1;uint32_t before[1024];memcpy(before,invalid,sizeof(before));
 if(NativeGpu::RcsPublicShader::PatchBatch(invalid,1024,0x410000)
  || memcmp(invalid,before,sizeof(invalid)))return 9;
 alignas(4096) uint32_t pages[NativeGpu::RcsDrawImage::PageCount][1024]={};
 uint32_t* pagePointers[NativeGpu::RcsDrawImage::PageCount]={};
 for(unsigned i=0;i<NativeGpu::RcsDrawImage::PageCount;i++)pagePointers[i]=pages[i];
 const uint32_t vertices[12]={
  0xbf400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0x3f400000,0x3f000000,0x3f800000};
 if(!NativeGpu::RcsResourceVertices::BuildAt(pagePointers,vertices,0x410000))return 10;
 uint32_t* combined=pagePointers[NativeGpu::RcsDrawImage::Batch];
 if(!NativeGpu::RcsPublicShader::PatchBatchWithVertex(combined,1024,0x411000,0x410000))
  return 11;
 if(combined[44]!=0x20001 || combined[239]!=0x11000
  || combined[248]!=0x11080)return 12;
 combined[0]^=1;
 if(NativeGpu::RcsPublicShader::PatchBatchWithVertex(combined,1024,0x411000,0x410000))
  return 13;
 puts("N100 bounded constant-green FS template and batch patch: PASS (CPU only)");
 return 0;
}
