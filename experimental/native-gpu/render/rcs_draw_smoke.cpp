// SPDX-License-Identifier: MIT
#include "RcsDrawImage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
using NativeGpu::RcsDrawImage;
// These are CPU construction/negative tests. No device is opened or submitted.
int main() {
 alignas(4096) uint32_t storage[8][1024];
 uint32_t* pages[8];
 for (unsigned i=0;i<8;++i) pages[i]=storage[i];
 memset(storage,0xa5,sizeof(storage));
 assert(!RcsDrawImage::Build(pages,4));
 assert(!RcsDrawImage::Build(pages,0,0x600000));
 assert(!RcsDrawImage::Build(nullptr,0));
 uint32_t* aligned=pages[2]; pages[2]++;
 assert(!RcsDrawImage::Build(pages,0)); pages[2]=aligned;
 for (unsigned i=0;i<8192;++i) assert(reinterpret_cast<uint32_t*>(storage)[i]==0xa5a5a5a5);
 uint32_t* saved=pages[1]; pages[1]=pages[0];
 assert(!RcsDrawImage::Build(pages,0)); pages[1]=saved;
 pages[1]=nullptr; assert(!RcsDrawImage::Build(pages,0)); pages[1]=saved;
 uint32_t vertices[4][12];
 for (unsigned v=0;v<4;++v) {
  assert(RcsDrawImage::Build(pages,v));
  assert(pages[0][RcsDrawImage::BatchDwords-1]==0x05000000); // MI_BATCH_BUFFER_END
  for (unsigned i=0;i<1024;++i) assert(pages[6][i]==RcsDrawImage::Poison);
  memcpy(vertices[v],pages[3],48);
  // Integer edge equations verify that exported sample coordinates are safely
  // on opposite sides of the triangle, independently of GPU rasterization.
  auto inside=[&](unsigned x,unsigned y) {
   for(unsigned i=0;i<3;++i) {
    unsigned j=(i+1)%3;
    int ax=(pages[3][i*4]==0xbf400000?4:28)*2;
    int ay=(pages[3][i*4+1]==0xbf400000?4:28)*2;
    int bx=(pages[3][j*4]==0xbf400000?4:28)*2;
    int by=(pages[3][j*4+1]==0xbf400000?4:28)*2;
    if((bx-ax)*(int(y*2+1)-ay)-(by-ay)*(int(x*2+1)-ax)<=0) return false;
   }
   return true;
  };
  assert(inside(RcsDrawImage::InteriorX(v),RcsDrawImage::InteriorY(v)));
  assert(!inside(RcsDrawImage::ExteriorX(v),RcsDrawImage::ExteriorY(v)));
  assert(!inside(0,0));
  assert(pages[3][3]==0x3f800000 && pages[3][7]==0x3f800000 && pages[3][11]==0x3f800000);
  assert(memcmp(pages[4],NativeGpu::RcsDrawData::Vs,sizeof(NativeGpu::RcsDrawData::Vs))==0);
  assert(memcmp(pages[5],NativeGpu::RcsDrawData::Fs,sizeof(NativeGpu::RcsDrawData::Fs))==0);
 }
 for(unsigned v=1;v<4;++v) assert(memcmp(vertices[0],vertices[v],48)!=0);
 puts("RCS draw CPU image checks passed (no hardware draw proof)");
}
