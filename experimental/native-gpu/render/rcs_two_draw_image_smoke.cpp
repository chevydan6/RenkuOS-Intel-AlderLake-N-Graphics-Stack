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
}
