// SPDX-License-Identifier: MIT
#include "RcsPublicShader.h"
#include "RcsDepthVertex.h"
#include "RcsRotateVertex.h"
#include "RcsFragCoordRed.h"
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
 struct RgbCase {uint32_t red,green,blue,pixel;};
 const RgbCase rgbAccepted[]={
  {0x3f800000,0x3e800000,0x3f000000,0xffff4080},
  {0x3f800000,0x3e800000,0x3f800000,0xffff40ff},
  {0x3f000000,0x3e800000,0x00000000,0xff804000},
  {0x00000000,0x3e800000,0x00000000,0xff004000},
  {0x3f000000,0x3f400000,0x3e800000,0xff80bf40}};
 for(const auto& c:rgbAccepted){
  shader[3]=shader[35]=c.red;
  shader[7]=shader[39]=c.green;
  shader[11]=shader[43]=c.blue;
  if(!NativeGpu::RcsPublicShader::ValidateFragment(shader,1024)
   || !NativeGpu::RcsPublicShader::ExpectedPixel(shader,1024,pixel)
   || pixel!=c.pixel)return 21;
 }
 shader[35]=0x3f800000;
 if(NativeGpu::RcsPublicShader::ValidateFragment(shader,1024))return 22;
 shader[35]=shader[3];shader[43]=0;
 if(NativeGpu::RcsPublicShader::ValidateFragment(shader,1024))return 23;
 shader[43]=shader[11];shader[3]=shader[35]=0x3f800001;
 if(NativeGpu::RcsPublicShader::ValidateFragment(shader,1024))return 24;
 shader[3]=shader[35]=0x3f800000;shader[11]=shader[43]=0xbf000000;
 if(NativeGpu::RcsPublicShader::ValidateFragment(shader,1024))return 25;
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
 alignas(4096) uint32_t vertexPage[1024]={};
 memcpy(vertexPage,NativeGpu::RcsDepthVertex::Words,
  sizeof(NativeGpu::RcsDepthVertex::Words));
 if(!NativeGpu::RcsPublicShader::ValidateVertexPage(vertexPage,1024))return 26;
 vertexPage[47]^=1;
 if(NativeGpu::RcsPublicShader::ValidateVertexPage(vertexPage,1024))return 27;
 vertexPage[47]^=1;vertexPage[1023]=1;
 if(NativeGpu::RcsPublicShader::ValidateVertexPage(vertexPage,1024))return 28;
 vertexPage[1023]=0;
 if(NativeGpu::RcsPublicShader::ValidateVertexPage(vertexPage,1023))return 29;
 memset(vertexPage,0,sizeof(vertexPage));
 memcpy(vertexPage,NativeGpu::RcsRotateVertex::Words,
  sizeof(NativeGpu::RcsRotateVertex::Words));
 auto program=NativeGpu::RcsPublicShader::IdentifyVertexPage(vertexPage,1024);
 if(program!=NativeGpu::RcsPublicShader::RotateHalfturn)return 30;
 uint32_t transformed[12]={};
 if(!NativeGpu::RcsPublicShader::ExpectedVertices(program,vertices,transformed)
  || transformed[0]!=0x3f400000 || transformed[1]!=0x3f400000
  || transformed[4]!=0xbf400000 || transformed[5]!=0x3f400000
  || transformed[8]!=0xbf400000 || transformed[9]!=0xbf400000)return 31;
 vertexPage[43]^=1;
 if(NativeGpu::RcsPublicShader::IdentifyVertexPage(vertexPage,1024)
  !=NativeGpu::RcsPublicShader::InvalidVertex)return 32;
 vertexPage[43]^=1;vertexPage[1023]=1;
 if(NativeGpu::RcsPublicShader::ValidateVertexPage(vertexPage,1024))return 33;
 alignas(4096) uint32_t gradientPage[1024]={};
 memcpy(gradientPage,NativeGpu::RcsFragCoordRed::Words,
  sizeof(NativeGpu::RcsFragCoordRed::Words));
 if(NativeGpu::RcsPublicShader::IdentifyFragmentPage(gradientPage,1024)
  !=NativeGpu::RcsPublicShader::FragCoordRed)return 34;
 gradientPage[29]=gradientPage[77]=0x3c002002; // gl_FragCoord.x / 128.
 if(NativeGpu::RcsPublicShader::IdentifyFragmentPage(gradientPage,1024)
  !=NativeGpu::RcsPublicShader::FragCoordRed)return 38;
 gradientPage[29]=gradientPage[77]=0x3d002002; // gl_FragCoord.x / 32.
 if(NativeGpu::RcsPublicShader::IdentifyFragmentPage(gradientPage,1024)
  !=NativeGpu::RcsPublicShader::FragCoordRed)return 39;
 gradientPage[77]=0x3c802002;
 if(NativeGpu::RcsPublicShader::IdentifyFragmentPage(gradientPage,1024)
  !=NativeGpu::RcsPublicShader::InvalidFragment)return 40;
 gradientPage[29]=gradientPage[77]=0x3d802002; // Unsupported 1/16 scale.
 if(NativeGpu::RcsPublicShader::IdentifyFragmentPage(gradientPage,1024)
  !=NativeGpu::RcsPublicShader::InvalidFragment)return 41;
 gradientPage[29]=gradientPage[77]=0x3c802002;
 gradientPage[87]^=1;
 if(NativeGpu::RcsPublicShader::IdentifyFragmentPage(gradientPage,1024)
  !=NativeGpu::RcsPublicShader::InvalidFragment)return 35;
 gradientPage[87]^=1;gradientPage[1023]=1;
 if(NativeGpu::RcsPublicShader::IdentifyFragmentPage(gradientPage,1024)
  !=NativeGpu::RcsPublicShader::InvalidFragment)return 36;
 gradientPage[1023]=0;
 gradientPage[5]=0x7c054220;
 gradientPage[28]=0xe17d0141;
 gradientPage[53]=0x78054220;
 gradientPage[76]=0xe07a0141;
 if(NativeGpu::RcsPublicShader::IdentifyFragmentPage(gradientPage,1024)
  !=NativeGpu::RcsPublicShader::FragCoordGreen)return 44;
 gradientPage[29]=gradientPage[77]=0x3c002002;
 if(NativeGpu::RcsPublicShader::IdentifyFragmentPage(gradientPage,1024)
  !=NativeGpu::RcsPublicShader::FragCoordGreen)return 45;
 gradientPage[5]^=1;
 if(NativeGpu::RcsPublicShader::IdentifyFragmentPage(gradientPage,1024)
  !=NativeGpu::RcsPublicShader::InvalidFragment)return 46;
 alignas(4096) uint32_t largeStorage[NativeGpu::RcsDrawImage64::PageCount][1024]={};
 uint32_t* largePages[NativeGpu::RcsDrawImage64::PageCount]={};
 for(unsigned i=0;i<NativeGpu::RcsDrawImage64::PageCount;i++)
  largePages[i]=largeStorage[i];
 if(!NativeGpu::RcsDrawImage64::Build(largePages,0)
  || !NativeGpu::RcsResourceVertices::PatchAddressOnly(largePages,0x410000)
  || !NativeGpu::RcsPublicShader::PatchBatchWithVertex(largePages[0],1024,
   0x411000,0x410000,true,NativeGpu::RcsPublicShader::FragCoordRed)
  || largePages[0][244]!=0x1f800003
  || largePages[0][245]!=0x00040006
  || largePages[0][248]!=0x110c0)return 37;
 if(!NativeGpu::RcsDrawImage64::Build(largePages,0)
  || !NativeGpu::RcsResourceVertices::PatchAddressOnly(largePages,0x410030)
  || !NativeGpu::RcsPublicShader::PatchBatchWithVertex(largePages[0],1024,
   0x411000,0x410030,true,NativeGpu::RcsPublicShader::FragCoordRed))return 42;
 if(!NativeGpu::RcsDrawImage64::Build(largePages,0)
  || NativeGpu::RcsResourceVertices::PatchAddressOnly(largePages,0x410ff0)
  || NativeGpu::RcsResourceVertices::PatchAddressOnly(largePages,0x410031)
  || NativeGpu::RcsPublicShader::PatchBatchWithVertex(largePages[0],1024,
   0x411000,0x410ff0,true,NativeGpu::RcsPublicShader::FragCoordRed))return 43;
 if(!NativeGpu::RcsDrawImage64::Build(largePages,0)
  || !NativeGpu::RcsResourceVertices::PatchAddressOnly(largePages,0x410000)
  || !NativeGpu::RcsPublicShader::PatchBatchWithVertex(largePages[0],1024,
   0x411000,0x410000,true,NativeGpu::RcsPublicShader::FragCoordGreen)
  || largePages[0][245]!=0x00040006
  || largePages[0][248]!=0x110c0)return 47;
 puts("N100 bounded FS/VS templates and batch patch: PASS (CPU only)");
 return 0;
}
