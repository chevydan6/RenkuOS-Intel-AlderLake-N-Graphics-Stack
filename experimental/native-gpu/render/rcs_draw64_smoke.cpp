// SPDX-License-Identifier: MIT
// Offline image construction check against the Mesa 26.3 64x64 packet artifact.
#include "RcsDrawImage64.h"
#include "RcsRequestReuse.h"
#include "draw/RcsDrawGenerated64.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main()
{
 using NativeGpu::RcsDrawImage64;
 namespace Data=NativeGpu::RcsDrawData64;
 alignas(4096) uint32_t storage[11][1024];
 uint32_t* pages[11];
 for(unsigned i=0;i<11;i++)pages[i]=storage[i];
 memset(storage,0xa5,sizeof(storage));
 assert(!RcsDrawImage64::Build(pages,4));
 assert(!RcsDrawImage64::Build(pages,0,0x500000));
 assert(!RcsDrawImage64::Build(nullptr,0));
 uint32_t* saved=pages[9];pages[9]=pages[8];
 assert(!RcsDrawImage64::Build(pages,0));pages[9]=saved;
 pages[10]++;assert(!RcsDrawImage64::Build(pages,0));pages[10]--;
 for(unsigned i=0;i<11*1024;i++)assert((&storage[0][0])[i]==0xa5a5a5a5);
 for(unsigned variant=0;variant<4;variant++){
  assert(RcsDrawImage64::Build(pages,variant));
  assert(memcmp(pages[0],Data::Batch,sizeof(Data::Batch))==0);
  assert(memcmp(pages[1],Data::Dynamic,sizeof(Data::Dynamic))==0);
  assert(memcmp(pages[2],Data::Surface,sizeof(Data::Surface))==0);
  assert(memcmp(pages[4],Data::Vs,sizeof(Data::Vs))==0);
  assert(memcmp(pages[5],Data::Fs,sizeof(Data::Fs))==0);
  for(unsigned page=6;page<10;page++)
   for(unsigned word=0;word<1024;word++)assert(pages[page][word]==0x19537bad);
  for(unsigned word=0;word<1024;word++)assert(pages[10][word]==0);
  assert(pages[3][3]==0x3f800000 && pages[3][7]==0x3f800000
   && pages[3][11]==0x3f800000);
  const int corner[4][2]={{8,8},{56,8},{56,56},{8,56}};
  for(int y=0;y<64;y++)for(int x=0;x<64;x++){
   bool inside=true;
   for(unsigned edge=0;edge<3;edge++){
    const int* a=corner[(variant+edge)&3];
    const int* b=corner[(variant+(edge+1)%3)&3];
    int cross=(b[0]-a[0])*(2*y+1-2*a[1])
     -(b[1]-a[1])*(2*x+1-2*a[0]);
    if(cross<=0)inside=false;
   }
   if(inside){unsigned index=unsigned(y*64+x);
    pages[6+index/1024][index%1024]=NativeGpu::RcsDrawImage::RenderedPixel;}
  }
  NativeGpu::RcsRequestReuse::Check check={};
  bool matched=NativeGpu::RcsRequestReuse::Inspect64(pages,variant,check);
  if(!matched)fprintf(stderr,"variant=%u checked=%u matched=%u interior=%u,%u\n",
   variant,check.checked,check.matched,check.interior&65535,check.interior>>16);
  assert(matched);
  unsigned inside=(check.interior>>16)*64+(check.interior&65535);
  pages[6+inside/1024][inside%1024]=NativeGpu::RcsDrawImage::Poison;
  assert(!NativeGpu::RcsRequestReuse::Inspect64(pages,variant,check));
 }
 puts("RCS 64x64 CPU packet image: PASS (no hardware submission)");
}
