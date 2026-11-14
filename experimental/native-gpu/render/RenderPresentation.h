// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RENDER_PRESENTATION_H
#define NATIVE_GPU_RENDER_PRESENTATION_H
#include "../RenderReport.h"
#include "RcsDrawImage.h"
#include <IntelN100ContextStatus.h>
#include <string.h>
namespace NativeGpu {
struct RenderPixels { uint32_t checked,hash; };
// Independent userspace inspection of a completed 32x32 CPU-visible snapshot.
// Hash ABI is FNV over 1024 little-endian words, not over 4096 bytes.
inline bool InspectRenderPixels(const uint8_t* pixels,unsigned variant,RenderPixels& result) {
 if(!pixels || variant>=4)return false;
 RenderPixels r={0,2166136261u};bool valid=true;
 for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
  uint32_t pixel;memcpy(&pixel,pixels+(y*32+x)*4,4);r.hash=(r.hash^pixel)*16777619u;
  bool border=x<3 || x>=29 || y<3 || y>=29;
  int distance=(variant&1)?int(x+y)-31:int(x)-int(y);
  bool sample=border || (x>=6 && x<26 && y>=6 && y<26 && (distance<=-3 || distance>=3));
  if(sample){
   bool covered=!border && (variant<2?distance>0:distance<0);
   if(pixel!=(covered?RcsDrawImage::RenderedPixel:RcsDrawImage::Poison))valid=false;
   ++r.checked;
  }
 }
 uint32_t interior,exterior;
 memcpy(&interior,pixels+(RcsDrawImage::InteriorY(variant)*32+RcsDrawImage::InteriorX(variant))*4,4);
 memcpy(&exterior,pixels+(RcsDrawImage::ExteriorY(variant)*32+RcsDrawImage::ExteriorX(variant))*4,4);
 result=r;
 return valid && interior==RcsDrawImage::RenderedPixel && exterior==RcsDrawImage::Poison;
}
class RenderCompletionGate {
 bool fFailed=false;uint32_t fSequence=0;
 bool Reject(){fFailed=true;return false;}
public:
 bool CanSubmit() const{return !fFailed;}
 uint32_t LastSequence() const{return fSequence;}
 void Fail(){fFailed=true;}
 bool Complete(const native_gpu_render_report& r,uint32_t mode) {
  if(fFailed)return false;
  uint32_t required=NATIVE_GPU_RENDER_INITIALIZED|NATIVE_GPU_RENDER_DEFAULT_VALID;
  IntelN100ContextStatus csb;
  if(r.version!=1 || r.mode!=mode || r.status || r.refusal
   || r.stage!=NATIVE_GPU_RENDER_STAGE_COMPLETE || !r.known_profile || !r.owner_granted
   || (r.state_flags&required)!=required || !r.sequence || r.sequence<=fSequence
   || r.observed_sequence!=r.sequence || r.observed_nonce!=r.nonce || r.observed_complement!=~r.nonce
   || r.eir || r.esr || r.fault || r.mapping || r.dma || r.bootstrap || r.group
   || r.bootstrap_error || r.group_error || r.safety_reason || r.csb_error || r.engine_error || r.reset_error
   || !csb.Arm(r.sequence,1+(r.sequence-1)%0x7fe,0)
   || csb.Observe(r.sequence,r.activation)!=IntelN100ContextStatus::Accepted
   || csb.Observe(r.sequence,r.completion)!=IntelN100ContextStatus::Accepted
   || csb.LastReport().state!=IntelN100ContextStatus::SwitchedAway)return Reject();
  fSequence=r.sequence;return true;
 }
 bool Draw(const native_gpu_render_command& c,unsigned variant) {
  if(fFailed)return false;
  const auto& r=c.report;RenderPixels p={};
  if(c.version!=1 || c.mode!=NATIVE_GPU_RENDER_DRAW || c.variant!=variant || r.variant!=variant
   || !(r.state_flags&NATIVE_GPU_RENDER_POOL_READY) || !InspectRenderPixels(c.output,variant,p)
   || r.checked_pixels!=p.checked || r.matched_pixels!=p.checked || r.target_hash!=p.hash
   || r.interior_xy!=((RcsDrawImage::InteriorY(variant)<<16)|RcsDrawImage::InteriorX(variant))
   || r.exterior_xy!=((RcsDrawImage::ExteriorY(variant)<<16)|RcsDrawImage::ExteriorX(variant)))return Reject();
  return Complete(r,NATIVE_GPU_RENDER_DRAW);
 }
};
}
#endif
