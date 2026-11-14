// SPDX-License-Identifier: MIT
#include "RcsRequestReuse.h"
#include <assert.h>
#include <stdio.h>
int main()
{
 using R=NativeGpu::RcsRequestReuse;using D=NativeGpu::RcsDrawImage;
 alignas(4096) static uint32_t context[16384],saved[14336],status[1024],pixels[1024];
 for(unsigned i=0;i<16384;i++)context[i]=0xaabbccdd;
 for(unsigned i=0;i<14336;i++)saved[i]=0x12340000+i;
 saved[1024+3]=0x90009;
 assert(R::Restore(context,saved,0x20000,0x12345000,224,0x80041000));
 assert(context[1024+3]==0x90008 && context[1024+5]==0 && context[1024+7]==224);
 assert(context[1024+9]==0x20000 && context[1024+0x33]==0x12345000);
 assert(context[14000]==saved[14000] && context[14335]==saved[14335]);
 for(unsigned i=14336;i<16384;i++)assert(context[i]==0xaabbccdd);
 for(unsigned cursor=0;cursor<12;cursor++){
  for(unsigned i=0;i<1024;i++)status[i]=0x12345678;
  assert(R::PoisonNext(status,cursor));
  for(unsigned slot=0;slot<12;slot++)
   assert(status[16+slot*2]==(slot==(cursor+1)%12 || slot==(cursor+2)%12?UINT32_MAX:0x12345678));
 }
 assert(!R::PoisonNext(status,12));
 uint32_t batch[6]={};R::BatchStart(batch);
 assert(batch[0]==0x04000001 && batch[1]==0x18800101 && !(batch[1]&(1U<<22)));
 assert(batch[2]==0x400000 && batch[3]==0 && batch[4]==0x04000000 && batch[5]==0);
 R::BatchStart(batch,0x412000);
 assert(batch[0]==0x04000001 && batch[1]==0x18800101);
 assert(batch[2]==0x412000 && batch[3]==0 && batch[4]==0x04000000 && batch[5]==0);
 for(unsigned v=0;v<4;v++){
  for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
   int d=(v&1)?int(x+y)-31:int(x)-int(y);
   bool inside=v<2?d>0:d<0;
   pixels[y*32+x]=x>=4 && x<28 && y>=4 && y<28 && inside?D::RenderedPixel:D::Poison;
  }
  R::Check check={};assert(R::Inspect(pixels,v,check));
  assert(check.checked>500 && check.checked==check.matched);
  unsigned inside=D::InteriorY(v)*32+D::InteriorX(v);pixels[inside]=D::Poison;
  assert(!R::Inspect(pixels,v,check));pixels[inside]=D::RenderedPixel;
  pixels[0]=D::RenderedPixel;assert(!R::Inspect(pixels,v,check));
  for(unsigned i=0;i<1024;i++)pixels[i]=D::Poison;
  assert(!R::Inspect(pixels,v,check));
 }
 puts("RCS saved-state, ring batch start, CSB wrap and target checks: PASS (CPU only)");
}
