// SPDX-License-Identifier: MIT
#include "RcsUserVertices.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
 using V=NativeGpu::RcsUserVertices;
 using D=NativeGpu::RcsDrawImage;
 alignas(4096) uint32_t storage[8][1024];
 uint32_t* pages[8]={};for(unsigned i=0;i<8;i++)pages[i]=storage[i];
 const uint32_t triangles[2][12]={
  {0xbf400000,0xbf400000,0x3f000000,0x3f800000,
   0x3f400000,0xbf400000,0x3f000000,0x3f800000,
   0x3f400000,0x3f400000,0x3f000000,0x3f800000},
  {0xbf300000,0xbf300000,0x3f000000,0x3f800000,
   0x3f300000,0xbf300000,0x3f000000,0x3f800000,
   0x00000000,0x3f300000,0x3f000000,0x3f800000}};
 for(const auto& vertices:triangles){
  V::Coord p[3]={};assert(V::Validate(vertices,p));
  assert(V::Build(pages,vertices));
  assert(memcmp(pages[D::Vertices],vertices,sizeof(vertices))==0);
  assert(pages[D::Batch][D::BatchDwords-1]==0x05000000);
  uint32_t pixels[1024]={};
  for(int y=0;y<32;y++)for(int x=0;x<32;x++){
   bool covered=true;
   for(unsigned i=0;i<3;i++){
    unsigned j=(i+1)%3;
    int edge=(p[j].x-p[i].x)*(2*y+1-2*p[i].y)
     -(p[j].y-p[i].y)*(2*x+1-2*p[i].x);
    if(edge<=0)covered=false;
   }
   pixels[y*32+x]=covered?D::RenderedPixel:D::Poison;
  }
  V::Check check={};assert(V::Inspect(pixels,vertices,check));
  assert(check.checked>500 && check.checked==check.matched);
  for(unsigned i=0;i<1024;i++)if(pixels[i]==D::RenderedPixel)pixels[i]=0xffff8000;
  assert(V::Inspect(pixels,vertices,check,0xffff8000));
  assert(!V::Inspect(pixels,vertices,check));
  for(unsigned i=0;i<1024;i++)if(pixels[i]==0xffff8000)pixels[i]=D::RenderedPixel;
  unsigned inside=(check.interior>>16)*32+(check.interior&65535);
  pixels[inside]=D::Poison;assert(!V::Inspect(pixels,vertices,check));
  pixels[inside]=D::RenderedPixel;
  pixels[0]=D::RenderedPixel;assert(!V::Inspect(pixels,vertices,check));
 }
 uint32_t invalid[12];memcpy(invalid,triangles[0],sizeof(invalid));
 invalid[0]=0x7fc00000;assert(!V::Validate(invalid));
 invalid[0]=triangles[0][0];invalid[3]=0;assert(!V::Validate(invalid));
 memcpy(invalid,triangles[0],sizeof(invalid));
 memcpy(invalid+4,invalid,sizeof(uint32_t)*4);assert(!V::Validate(invalid));
 memset(storage,0xa5,sizeof(storage));
 assert(!V::Build(pages,invalid));
 for(unsigned i=0;i<8;i++)for(unsigned j=0;j<1024;j++)assert(storage[i][j]==0xa5a5a5a5);
 // Independently rasterize a deterministic spread of admitted grid triangles.
 static constexpr uint32_t grid[7]={0xbf400000,0xbf000000,0xbe800000,
  0x00000000,0x3e800000,0x3f000000,0x3f400000};
 uint32_t state=0x91546d1u,accepted=0;
 for(unsigned trial=0;trial<1200;trial++){
  uint32_t words[12]={};int selected[6]={};
  for(unsigned i=0;i<6;i++){
   state=state*1664525u+1013904223u;
   selected[i]=int((state>>16)%7);
   words[(i/2)*4+(i&1)]=grid[selected[i]];
  }
  for(unsigned i=0;i<3;i++){words[i*4+2]=0x3f000000;words[i*4+3]=0x3f800000;}
  V::Coord points[3]={};if(!V::Validate(words,points))continue;
  accepted++;
  for(unsigned i=0;i<3;i++){
   assert(points[i].x==4+4*selected[2*i]);
   assert(points[i].y==4+4*selected[2*i+1]);
  }
  uint32_t pixels[1024]={};
  for(int y=0;y<32;y++)for(int x=0;x<32;x++){
   bool covered=true;
   for(unsigned i=0;i<3;i++){
    unsigned j=(i+1)%3;
    int edge=(points[j].x-points[i].x)*(2*y+1-2*points[i].y)
     -(points[j].y-points[i].y)*(2*x+1-2*points[i].x);
    if(edge<=0)covered=false;
   }
   pixels[y*32+x]=covered?D::RenderedPixel:D::Poison;
  }
  V::Check check={};assert(V::Inspect(pixels,words,check));
 }
 assert(accepted>100);
 puts("RCS v2 user vertex validation, image construction, and readback checks: PASS (CPU only)");
}
