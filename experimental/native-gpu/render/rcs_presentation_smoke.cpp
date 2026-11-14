// SPDX-License-Identifier: MIT
#include "RenderPresentation.h"
#include <assert.h>
#include <stdio.h>
using namespace NativeGpu;
static native_gpu_render_command Frame(uint32_t sequence,unsigned variant=0) {
 native_gpu_render_command c={};c.version=1;c.mode=NATIVE_GPU_RENDER_DRAW;c.variant=variant;
 auto& r=c.report;r.version=1;r.mode=c.mode;r.stage=NATIVE_GPU_RENDER_STAGE_COMPLETE;
 r.variant=variant;r.sequence=r.observed_sequence=sequence;r.nonce=r.observed_nonce=123;r.observed_complement=~r.nonce;
 r.known_profile=r.owner_granted=1;r.state_flags=7;
 uint32_t tag=1+(sequence-1)%0x7fe;
 r.activation=(uint64_t(tag)<<15)|(uint64_t(0x7ff)<<47);
 r.completion=(uint64_t(0x7ff)<<15)|(uint64_t(tag)<<47);
 for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
  int d=(variant&1)?int(x+y)-31:int(x)-int(y);
  bool covered=variant<2?d>0:d<0;
  uint32_t pixel=x>=4 && x<28 && y>=4 && y<28 && covered?RcsDrawImage::RenderedPixel:RcsDrawImage::Poison;
  memcpy(c.output+(y*32+x)*4,&pixel,4);
 }
 RenderPixels p={};assert(InspectRenderPixels(c.output,variant,p));
 r.checked_pixels=r.matched_pixels=p.checked;r.target_hash=p.hash;
 r.interior_xy=(RcsDrawImage::InteriorY(variant)<<16)|RcsDrawImage::InteriorX(variant);
 r.exterior_xy=(RcsDrawImage::ExteriorY(variant)<<16)|RcsDrawImage::ExteriorX(variant);
 return c;
}
int main() {
 for(unsigned v=0;v<4;v++){RenderCompletionGate g;auto frame=Frame(10+v,v);assert(g.Draw(frame,v));}
 RenderCompletionGate gate;auto c=Frame(2);assert(gate.Draw(c,0));
 assert(!gate.Draw(c,0));assert(!gate.CanSubmit()); // duplicate completion latches off
 RenderCompletionGate corrupt; c=Frame(3);c.output[0]^=1;assert(!corrupt.Draw(c,0));
 RenderCompletionGate wrongEngine;c=Frame(4);c.report.completion|=3ULL<<3;assert(!wrongEngine.Draw(c,0));
 RenderCompletionGate stale;c=Frame(5);c.report.observed_nonce++;assert(!stale.Draw(c,0));
 RenderCompletionGate counts;c=Frame(6);c.report.matched_pixels--;assert(!counts.Draw(c,0));
 RenderCompletionGate variant;c=Frame(7);assert(!variant.Draw(c,1));
 puts("RCS presentation policy passed (CPU fixtures only)");
}
