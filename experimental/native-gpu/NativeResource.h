// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RESOURCE_H
#define NATIVE_GPU_RESOURCE_H
#include <stdint.h>

namespace NativeGpu {
// CPU policy for one file-local resource namespace and one GPU VM epoch. This
// does not allocate pages or edit PTEs. The owner alone may call RetireEpoch
// after proving that no GPU reference remains. Binding is sealed at Publish
// until a hardware-tested TLB invalidation path exists. The caller serializes
// all operations for this table, including lookups.
class ResourceTable {
public:
 static constexpr unsigned SlotCount=64;
 ResourceTable(uint64_t base,uint64_t length)
  :fBase(base),fLength(length),fPublished(false),fValid((base&4095)==0
    && (length&4095)==0 && length!=0 && base<(1ULL<<48)
    && length<=(1ULL<<48)-base) {
  for(auto& slot:fSlots)slot.generation=1;
 }
 bool Create(uint64_t bytes,uint32_t& handle,uint64_t& actual) {
  if(!fValid || fPublished || !bytes || bytes>UINT64_MAX-4095)return false;
  uint64_t rounded=(bytes+4095)&~UINT64_C(4095);
  if(rounded>fLength)return false;
  for(unsigned i=0;i<SlotCount;i++){
   Slot& slot=fSlots[i];
   if(slot.live || slot.retained || slot.expired)continue;
   slot.bytes=rounded;slot.live=true;
   handle=(slot.generation<<7)|(i+1);
   actual=rounded;return true;
  }
  return false;
 }
 bool Bind(uint32_t handle,uint64_t gpu,uint64_t offset,uint64_t bytes) {
  Slot* slot=Live(handle);
  if(!slot || fPublished || slot->bound || !bytes
   || ((gpu|offset|bytes)&4095) || gpu<fBase || gpu-fBase>fLength
   || bytes>fLength-(gpu-fBase) || offset>slot->bytes
   || bytes>slot->bytes-offset)return false;
  for(const auto& other:fSlots){
   if(!other.bound)continue;
   if(gpu<other.gpu+other.boundBytes && other.gpu<gpu+bytes)return false;
  }
  slot->gpu=gpu;slot->offset=offset;slot->boundBytes=bytes;
  slot->bound=true;return true;
 }
 bool Range(uint32_t handle,uint64_t offset,uint64_t bytes) const {
  const Slot* slot=Live(handle);
  return slot && bytes && offset<=slot->bytes && bytes<=slot->bytes-offset;
 }
 bool Address(uint32_t handle,uint64_t offset,uint64_t bytes,uint64_t& address) const {
  const Slot* slot=Live(handle);
  if(!slot || !slot->bound || !bytes || offset<slot->offset)return false;
  uint64_t relative=offset-slot->offset;
  if(relative>slot->boundBytes || bytes>slot->boundBytes-relative)return false;
  address=slot->gpu+relative;return true;
 }
 bool Publish() {if(!fValid || fPublished)return false;fPublished=true;return true;}
 bool Destroy(uint32_t handle) {
  Slot* slot=Live(handle);
  if(!slot)return false;
  slot->live=false;
  if(fPublished)slot->retained=true;
  else {slot->bound=false;slot->bytes=0;Advance(*slot);}
  return true;
 }
 unsigned RetainedCount() const {
  unsigned count=0;for(const auto& slot:fSlots)count+=slot.retained;return count;
 }
 bool RetireEpoch(bool gpuRetired) {
  if(!gpuRetired)return false;
  for(auto& slot:fSlots){
   if(slot.live || slot.retained){
    slot.live=false;slot.retained=false;slot.bound=false;slot.bytes=0;
    Advance(slot);
   }
  }
  fPublished=false;return true;
 }
private:
 struct Slot {
  uint64_t bytes=0,gpu=0,offset=0,boundBytes=0;
  uint32_t generation=0;
  bool live=false,bound=false,retained=false,expired=false;
 };
 static constexpr uint32_t MaxGeneration=(1U<<25)-1;
 static void Advance(Slot& slot) {
  if(slot.generation==MaxGeneration)slot.expired=true;
  else ++slot.generation;
 }
 Slot* Live(uint32_t handle) {
  return const_cast<Slot*>(static_cast<const ResourceTable*>(this)->Live(handle));
 }
 const Slot* Live(uint32_t handle) const {
  unsigned index=handle&127;
  if(index<1 || index>SlotCount)return nullptr;
  const Slot& slot=fSlots[index-1];
  return slot.live && slot.generation==(handle>>7)?&slot:nullptr;
 }
 uint64_t fBase,fLength;
 bool fPublished,fValid;
 Slot fSlots[SlotCount];
};
}
#endif
