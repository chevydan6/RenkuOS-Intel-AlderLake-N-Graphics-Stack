// SPDX-License-Identifier: MIT
#include "RcsUserVertices.h"
#include <stdio.h>

int main()
{
 alignas(4096) uint32_t storage[11][1024]={};
 uint32_t* pages[11]={};
 for(unsigned i=0;i<11;i++)pages[i]=storage[i];
 const uint32_t vertices[12]={
  0xbf400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0x3f400000,0x3f000000,0x3f800000};
 for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){
  unsigned index=y*64+x;
  unsigned red=((2*x+1)*255+64)/128;
  pages[6+index/1024][index%1024]=
   x>y && x>=8 && x<56 && y>=8 && y<56
   ?0xff000000u|(red<<16):NativeGpu::RcsDrawImage::Poison;
 }
 NativeGpu::RcsUserVertices::Check check={};
 if(!NativeGpu::RcsUserVertices::Inspect64(pages,vertices,check,0,64)
  || check.checked<2000 || check.matched!=check.checked)return 1;
 unsigned index=10*64+20;
 pages[6+index/1024][index%1024]^=0x00010000;
 if(NativeGpu::RcsUserVertices::Inspect64(pages,vertices,check,0,64)
  || check.matched==check.checked)return 2;
 const unsigned denominators[]={32,64,128};
 for(unsigned denominator:denominators){
  for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){
   unsigned index=y*64+x;
   unsigned red=((2*x+1)*255+denominator)/(2*denominator);
   if(red>255)red=255;
   pages[6+index/1024][index%1024]=
    x>y && x>=8 && x<56 && y>=8 && y<56
    ?0xff000000u|(red<<16):NativeGpu::RcsDrawImage::Poison;
  }
  if(!NativeGpu::RcsUserVertices::Inspect64(
    pages,vertices,check,0,denominator)
   || check.matched!=check.checked)return 3;
  for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){
   unsigned index=y*64+x;
   unsigned green=((2*x+1)*255+denominator)/(2*denominator);
   if(green>255)green=255;
   pages[6+index/1024][index%1024]=
    x>y && x>=8 && x<56 && y>=8 && y<56
    ?0xff000000u|(green<<8):NativeGpu::RcsDrawImage::Poison;
  }
  if(!NativeGpu::RcsUserVertices::Inspect64(
    pages,vertices,check,0,denominator,true)
   || check.matched!=check.checked)return 4;
  if(NativeGpu::RcsUserVertices::Inspect64(
    pages,vertices,check,0,denominator,false)
   || check.matched==check.checked)return 5;
 }
 puts("FragCoord 64x64 pixel inspection: PASS");
 return 0;
}
