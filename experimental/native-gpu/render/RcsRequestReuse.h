// SPDX-License-Identifier: MIT
// Copyright © 2014 Intel Corporation
// Native adaptation Copyright (c) 2026 chevydan6
#ifndef NATIVE_GPU_RCS_REQUEST_REUSE_H
#define NATIVE_GPU_RCS_REQUEST_REUSE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "RcsDrawImage.h"
namespace NativeGpu {
// CPU construction/inspection only. Caller first proves switch-away, visibility,
// idle, owner/DMA and normal power release. None of these helpers grants reuse.
class RcsRequestReuse {
public:
 static constexpr size_t SavedBytes=14*4096;
 static bool Restore(uint32_t* context,const uint32_t* saved,uint64_t ring,
  uint64_t root,uint32_t tail,uint32_t rpcs) {
  if(!context || !saved || (ring&4095) || ring>(1ULL<<32)-4096
   || (root&4095) || root>(1ULL<<39)-4096 || !tail || (tail&7) || tail>4096-64)return false;
  // Preserve ALL 14 saved pages; the two WA pages retain their original bytes.
  // intel_lrc.c init_common_regs: masked disable ENGINE_CTX_RESTORE_INHIBIT.
  memcpy(context,saved,SavedBytes);
  uint32_t* regs=context+1024;
  regs[3]=(regs[3]|0x10000U)&~1U;
  // intel_lrc.c lrc_update_regs, fixed 4KiB ring and admitted PML4/RPCS.
  regs[5]=0;regs[7]=tail;regs[9]=uint32_t(ring);regs[11]=1;
  regs[0x31]=uint32_t(root>>32);regs[0x33]=uint32_t(root);regs[0x43]=rpcs;
  return true;
 }
 static void BatchStart(uint32_t* words,uint32_t batchGpu=0x400000) {
  // gen8_engine_cs.c gen8_emit_bb_start, normal preemptible/nonsecure PPGTT.
  // First-level start returns to ring after MI_BATCH_BUFFER_END; bit22 (second
  // level) is deliberately absent. Disable arbitration again at ring return.
  const uint32_t packet[]={0x04000001,0x18800101,batchGpu,0,0x04000000,0};
  memcpy(words,packet,sizeof(packet));
 }
 static bool PoisonNext(uint32_t* status,uint32_t cursor) {
  if(!status || cursor>=12)return false;
  // Gen12 engine CSB starts at byte0x40, 12 eight-byte entries.
  for(unsigned i=1;i<=2;i++){
   unsigned slot=(cursor+i)%12;status[0x40/4+slot*2]=UINT32_MAX;
   status[0x40/4+slot*2+1]=UINT32_MAX;
  }
  return true;
 }
 struct Check {uint32_t checked,matched,hash,interior,exterior;};
 static bool Inspect(const volatile uint32_t* target,unsigned variant,Check& check,
  uint32_t renderedPixel=RcsDrawImage::RenderedPixel) {
  if(!target || variant>=4)return false;
  Check r={0,0,2166136261u,0,0};
  r.interior=(RcsDrawImage::InteriorY(variant)<<16)|RcsDrawImage::InteriorX(variant);
  r.exterior=(RcsDrawImage::ExteriorY(variant)<<16)|RcsDrawImage::ExteriorX(variant);
  for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
   const uint32_t observed=target[y*32+x];r.hash=(r.hash^observed)*16777619u;
   // Pixel centers, at least two pixels away from every raster edge. Check
   // both halves inside the quad and untouched outer border, omit edge bands.
   bool outer=x<3 || x>=29 || y<3 || y>=29;
   bool central=x>=6 && x<26 && y>=6 && y<26;
   int diagonal=(variant&1)?int(x+y)-31:int(x)-int(y);
   bool clearEdge=diagonal<=-3 || diagonal>=3;
   if(!outer && !(central && clearEdge))continue;
   bool inside=variant<2?diagonal>0:diagonal<0;
   uint32_t expected=!outer && inside?renderedPixel:RcsDrawImage::Poison;
   ++r.checked;r.matched+=observed==expected;
  }
  check=r;return r.checked && r.matched==r.checked;
 }
 static bool Inspect64(uint32_t* const* pages,unsigned variant,Check& check,
  uint32_t renderedPixel=RcsDrawImage::RenderedPixel) {
  if(!pages || variant>=4)return false;
  for(unsigned p=6;p<10;p++)if(!pages[p])return false;
  Check r={0,0,2166136261u,0,0};
  r.interior=((RcsDrawImage::InteriorY(variant)*2)<<16)
   |(RcsDrawImage::InteriorX(variant)*2);
  r.exterior=((RcsDrawImage::ExteriorY(variant)*2)<<16)
   |(RcsDrawImage::ExteriorX(variant)*2);
  for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){
   unsigned index=y*64+x;
   uint32_t observed=pages[6+index/1024][index%1024];
   r.hash=(r.hash^observed)*16777619u;
   bool outer=x<6 || x>=58 || y<6 || y>=58;
   bool central=x>=12 && x<52 && y>=12 && y<52;
   int diagonal=(variant&1)?int(x+y)-63:int(x)-int(y);
   bool clearEdge=diagonal<=-6 || diagonal>=6;
   if(!outer && !(central && clearEdge))continue;
   bool inside=variant<2?diagonal>0:diagonal<0;
   uint32_t expected=!outer && inside?renderedPixel:RcsDrawImage::Poison;
   ++r.checked;r.matched+=observed==expected;
  }
  check=r;return r.checked>=2000 && r.matched==r.checked;
 }
};
}
#endif
