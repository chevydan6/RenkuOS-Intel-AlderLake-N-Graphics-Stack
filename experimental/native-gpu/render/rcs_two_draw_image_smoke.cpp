// SPDX-License-Identifier: MIT
#include "RcsUserVertices.h"
#include <assert.h>

int main() {
 // Union of two independently captured N100 shape-0 and shape-2 triangles.
 static constexpr uint32_t rows[32]={
  0,0,0,0,0x0ffffff0,0x0fffffe0,0x0fffffc0,0x0fffff80,
  0x0fffff00,0x0ffffe00,0x0ffffc00,0x0ffff800,
  0x0ffff800,0x0ffffc00,0x0ffffc00,0x0ffffe00,
  0x0fffff00,0x0fffff00,0x0fffff80,0x0fffff80,
  0x0fffffc0,0x0fffffe0,0x0fffffe0,0x0ffffff0,
  0x0f000000,0x0e000000,0x0c000000,0x08000000,0,0,0,0};
 static constexpr uint32_t first[12]={
  0xbf400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0x3f400000,0x3f000000,0x3f800000};
 static constexpr uint32_t second[12]={
  0xbf400000,0x3f000000,0x3f000000,0x3f800000,
  0x00000000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0x3f000000,0x3f000000,0x3f800000};
 uint32_t pixels[1024];
 for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++)
  pixels[y*32+x]=(rows[y]&(1u<<x))?0xffff4000:0x19537bad;
 NativeGpu::RcsUserVertices::Check check={};
 assert(NativeGpu::RcsUserVertices::InspectPair(pixels,first,second,check,
  0xffff4000));
 assert(check.checked>500 && check.checked==check.matched);
 pixels[10*32+25]=0x19537bad;
 assert(!NativeGpu::RcsUserVertices::InspectPair(pixels,first,second,check,
  0xffff4000));
 pixels[10*32+25]=0xffff4000;
 pixels[0]=0;
 assert(!NativeGpu::RcsUserVertices::InspectPair(pixels,first,second,check,
  0xffff4000));

 // Same two positions at 64x64, from the independent N100 single captures.
 static constexpr uint64_t rows64[64]={
  0,0,0,0,0,0,0,0,
  0x00ffffffffffff00ULL,0x00fffffffffffe00ULL,0x00fffffffffffc00ULL,0x00fffffffffff800ULL,
  0x00fffffffffff000ULL,0x00ffffffffffe000ULL,0x00ffffffffffc000ULL,0x00ffffffffff8000ULL,
  0x00ffffffffff0000ULL,0x00fffffffffe0000ULL,0x00fffffffffc0000ULL,0x00fffffffff80000ULL,
  0x00fffffffff00000ULL,0x00ffffffffe00000ULL,0x00ffffffffc00000ULL,0x00ffffffff800000ULL,
  0x00ffffffffc00000ULL,0x00ffffffffe00000ULL,0x00ffffffffe00000ULL,0x00fffffffff00000ULL,
  0x00fffffffff00000ULL,0x00fffffffff80000ULL,0x00fffffffffc0000ULL,0x00fffffffffc0000ULL,
  0x00fffffffffe0000ULL,0x00fffffffffe0000ULL,0x00ffffffffff0000ULL,0x00ffffffffff8000ULL,
  0x00ffffffffff8000ULL,0x00ffffffffffc000ULL,0x00ffffffffffc000ULL,0x00ffffffffffe000ULL,
  0x00fffffffffff000ULL,0x00fffffffffff000ULL,0x00fffffffffff800ULL,0x00fffffffffff800ULL,
  0x00fffffffffffc00ULL,0x00fffffffffffe00ULL,0x00fffffffffffe00ULL,0x00ffffffffffff00ULL,
  0x00ff000000000000ULL,0x00fe000000000000ULL,0x00fc000000000000ULL,0x00f8000000000000ULL,
  0x00f0000000000000ULL,0x00e0000000000000ULL,0x00c0000000000000ULL,0x0080000000000000ULL,
  0,0,0,0,0,0,0,0};
 uint32_t image64[4096];
 for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++)
  image64[y*64+x]=(rows64[y]&(1ULL<<x))?0xffff4000:0x19537bad;
 uint32_t* pages[11]={};
 for(unsigned p=0;p<4;p++)pages[6+p]=image64+p*1024;
 assert(NativeGpu::RcsUserVertices::InspectPair64(pages,first,second,check,
  0xffff4000));
 assert(check.checked>2000 && check.checked==check.matched
  && check.hash==0xbb1f873b);
 image64[20*64+50]=0x19537bad;
 assert(!NativeGpu::RcsUserVertices::InspectPair64(pages,first,second,check,
  0xffff4000));
}
