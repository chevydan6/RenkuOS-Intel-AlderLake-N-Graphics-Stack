// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RESOURCE_ARENA_H
#define NATIVE_GPU_RESOURCE_ARENA_H
#include <stdint.h>

namespace NativeGpu {
// Logical BOs over pages the hardware owner bound before its first GPU
// submission. This class never changes a PTE or owns physical backing.
// The owner serializes every call and alone supplies the retirement proof.
class ResourceArena {
public:
 static constexpr unsigned MaxPages=64, MaxBuffers=64, MaxSessions=8;
 ResourceArena(uint64_t base,unsigned pages)
  :fBase(base),fPages(pages),fValid(!(base&4095) && pages>0
   && pages<=MaxPages && base<(1ULL<<48)
   && uint64_t(pages)*4096<=(1ULL<<48)-base) {}
 bool Open(uint64_t& token) {
  if(!fValid)return false;
  for(unsigned i=0;i<MaxSessions;i++){
   Session& s=fSessions[i];
   if(s.live || s.closing || s.expired)continue;
   s.live=true;token=(s.generation<<8)|(i+1);return true;
  }
  return false;
 }
 bool Create(uint64_t token,uint64_t bytes,uint32_t& handle,uint64_t& gpu,uint64_t& actual) {
  if(!LiveSession(token) || !bytes || bytes>UINT64_MAX-4095)return false;
  uint64_t rounded=(bytes+4095)&~UINT64_C(4095);
  if(rounded>uint64_t(fPages)*4096)return false;
  unsigned count=unsigned(rounded/4096),slot=MaxBuffers;
  for(unsigned i=0;i<MaxBuffers;i++)
   if(!fBuffers[i].live && !fBuffers[i].expired){slot=i;break;}
  if(slot==MaxBuffers)return false;
  for(unsigned first=0;first+count<=fPages;first++){
   bool occupied=false;
   for(const Buffer& b:fBuffers)
    if(b.live && first<b.first+b.pages && b.first<first+count){occupied=true;break;}
   if(occupied)continue;
   Buffer& b=fBuffers[slot];b.live=true;b.owner=token;
   b.first=first;b.pages=count;b.bytes=rounded;
   handle=(b.generation<<7)|(slot+1);
   gpu=fBase+uint64_t(first)*4096;actual=rounded;return true;
  }
  return false;
 }
 bool Resolve(uint64_t token,uint32_t handle,uint64_t offset,uint64_t bytes,
  uint64_t& gpu) const {
  if(!LiveSession(token) || !bytes)return false;
  const Buffer* b=LiveBuffer(token,handle);
  if(!b || offset>b->bytes || bytes>b->bytes-offset)return false;
  gpu=fBase+uint64_t(b->first)*4096+offset;return true;
 }
 bool Destroy(uint64_t token,uint32_t handle,bool gpuRetired) {
  if(!gpuRetired || !LiveSession(token))return false;
  Buffer* b=LiveBuffer(token,handle);if(!b)return false;
  Release(*b);return true;
 }
 // A failed/unknown GPU operation leaves allocations resident and makes the
 // closing session inaccessible. They can be reclaimed only after reset or
 // completion proves that no GPU reference remains.
 bool Close(uint64_t token,bool gpuRetired) {
  Session* s=LiveSession(token);if(!s)return false;
  if(!gpuRetired){s->live=false;s->closing=true;return true;}
  for(Buffer& b:fBuffers)if(b.live && b.owner==token)Release(b);
  s->live=false;Advance(*s);return true;
 }
 bool RetireOrphans(bool gpuRetired) {
  if(!gpuRetired)return false;
  for(Session& s:fSessions)if(s.closing){
   for(Buffer& b:fBuffers)if(b.live && b.owner==Token(s))Release(b);
   s.closing=false;Advance(s);
  }
  return true;
 }
private:
 struct Session {
  uint64_t generation=1;
  bool live=false,closing=false,expired=false;
 };
 struct Buffer {
  uint64_t owner=0,bytes=0;
  uint32_t generation=1;
  unsigned first=0,pages=0;
  bool live=false,expired=false;
 };
 static void Advance(Session& s) {
  if(s.generation==(UINT64_MAX>>8))s.expired=true;
  else ++s.generation;
 }
 static void Release(Buffer& b) {
  b.live=false;b.owner=0;b.bytes=0;b.first=b.pages=0;
  if(b.generation==((1U<<25)-1))b.expired=true;
  else ++b.generation;
 }
 uint64_t Token(const Session& s) const {
  return (s.generation<<8)|unsigned(&s-fSessions+1);
 }
 Session* LiveSession(uint64_t token) {
  return const_cast<Session*>(static_cast<const ResourceArena*>(this)->LiveSession(token));
 }
 const Session* LiveSession(uint64_t token) const {
  unsigned index=token&255;
  if(index<1 || index>MaxSessions)return nullptr;
  const Session& s=fSessions[index-1];
  return s.live && s.generation==(token>>8)?&s:nullptr;
 }
 Buffer* LiveBuffer(uint64_t token,uint32_t handle) {
  return const_cast<Buffer*>(static_cast<const ResourceArena*>(this)->LiveBuffer(token,handle));
 }
 const Buffer* LiveBuffer(uint64_t token,uint32_t handle) const {
  unsigned index=handle&127;
  if(index<1 || index>MaxBuffers)return nullptr;
  const Buffer& b=fBuffers[index-1];
  return b.live && b.owner==token && b.generation==(handle>>7)?&b:nullptr;
 }
 uint64_t fBase;
 unsigned fPages;
 bool fValid;
 Session fSessions[MaxSessions];
 Buffer fBuffers[MaxBuffers];
};
}
#endif
