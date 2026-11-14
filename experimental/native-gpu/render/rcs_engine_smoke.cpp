// SPDX-License-Identifier: MIT
#include "RcsEngine.h"
#include <IntelN100CsbReader.h>
#include <assert.h>
#include <stdio.h>
#include <vector>
struct Write { uint32_t offset, value; };
struct Fixture {
 uint64_t time = 0;
 uint32_t hws = 0, cursor = 0, esr = 0;
 bool safe = true, failLoad = false, visibility = true;
 NativeGpu::RcsEngine* engine = nullptr;
 std::vector<Write> writes;
 std::vector<uint32_t> reads;
 static uint64_t Now(void* p) { return ++static_cast<Fixture*>(p)->time; }
 static bool Safe(void* p) { return static_cast<Fixture*>(p)->safe; }
 static bool Visible(void* p) { return static_cast<Fixture*>(p)->visibility; }
 static bool Read(void* p, uint32_t offset, uint32_t& value) {
  auto& f = *static_cast<Fixture*>(p);
  f.reads.push_back(offset);
  switch (offset) {
   case 0x203c: value=0; break;
   case 0x209c: value=0x200; break;
   case 0x2080: value=f.hws; break;
   case 0x23a0: value=f.cursor; break;
   case 0x20b8: value=f.esr; break;
   default: assert(false);
  }
  return true;
 }
 static bool Store(void* p, uint32_t offset, uint32_t value) {
  auto& f = *static_cast<Fixture*>(p);
  f.writes.push_back({offset,value});
  if (offset==0x2080) f.hws=value;
  if (offset==0x23a0) f.cursor=value&0xffff;
  if (offset==0x2550) {
   assert(f.engine && f.engine->LastReport().loadAttempted);
   assert(f.engine->LastReport().attemptedOffset==offset);
   return !f.failLoad;
  }
  return true;
 }
 NativeGpu::RcsEngine::IO Io() { return {this,Now,Safe,Read,Store,Visible}; }
};
static uint64_t Event(uint32_t engineClass, bool away) {
 return (uint64_t(engineClass)<<3) | (uint64_t(away?0x7ff:1)<<15)
  | (uint64_t(away?1:0x7ff)<<47);
}
struct CsbFixture {
 uint64_t time=0;
 uint32_t engineClass=0;
 static uint64_t Now(void* p) { return ++static_cast<CsbFixture*>(p)->time; }
 static bool Safe(void*) { return true; }
 static bool Tail(void*,uint32_t offset,uint32_t& value) {
  assert(offset==kN100CsbWrite); value=1; return true;
 }
 static bool Entry(void* p,uint32_t offset,uint64_t& value) {
  assert(offset==0x40 || offset==0x48);
  value=Event(static_cast<CsbFixture*>(p)->engineClass,offset==0x48);
  return true;
 }
 static void Relax(void*) {}
 IntelN100CsbReader::Io Io() { return {this,Now,Safe,Tail,Entry,Safe,Relax}; }
};

int main() {
 uint64_t descriptor=0;
 assert(intel_n100_rcs_descriptor(0x46d18086,0,0x40000,1,&descriptor));
 assert(descriptor==(0x40000ULL|0x11dULL|(1ULL<<37)));
 for (uint32_t tag : {0U,0x7ffU})
  assert(!intel_n100_rcs_descriptor(0x46d18086,0,0x40000,tag,&descriptor));
 assert(!intel_n100_rcs_descriptor(0x46d18086,1,0x40000,1,&descriptor));
 assert(!intel_n100_rcs_descriptor(0,0,0x40000,1,&descriptor));
 assert(!intel_n100_rcs_descriptor(0x46d18086,0,0xffff1000,1,&descriptor));
 assert(!intel_n100_rcs_descriptor(0x46d18086,0,0x40001,1,&descriptor));
 assert(intel_n100_rcs_descriptor(0x46d18086,0,0xffff0000,1,&descriptor));
 assert(intel_n100_rcs_descriptor(0x46d18086,0,0x40000,1,&descriptor));
 Fixture f; NativeGpu::RcsEngine engine; f.engine=&engine;
 assert(f.writes.empty() && engine.LastReport().stage==NativeGpu::RcsEngine::Empty);
 assert(engine.Initialize(f.Io(),0x46d18086,0,0x20000,10000));
 const Write init[]={{0x23a0,0xffff0b0b},{0x2098,UINT32_MAX},
  {0x229c,0x00080008},{0x209c,0x01000000},{0x2080,0x20000},
  {0x20b4,UINT32_MAX},{0x20b0,UINT32_MAX},{0x23a0,0xffff0b0b}};
 assert(f.writes.size()==sizeof(init)/sizeof(init[0]));
 for (size_t i=0;i<f.writes.size();++i)
  assert(f.writes[i].offset==init[i].offset && f.writes[i].value==init[i].value);
 const uint32_t initReads[]={0x203c,0x209c,0x23a0,0x2080,0x20b8,0x23a0};
 assert(f.reads.size()==sizeof(initReads)/sizeof(initReads[0]));
 for (size_t i=0;i<f.reads.size();++i) assert(f.reads[i]==initReads[i]);
 assert(engine.Publish(f.Io(),descriptor));
 const Write publish[]={{0x2518,0},{0x251c,0},{0x2510,uint32_t(descriptor)},
  {0x2514,uint32_t(descriptor>>32)},{0x2550,1}};
 for (size_t i=0;i<5;++i)
  assert(f.writes[i+8].offset==publish[i].offset && f.writes[i+8].value==publish[i].value);
 assert(engine.Rearm(f.Io(),20000));
 assert(engine.Publish(f.Io(),descriptor));
 Fixture failed; NativeGpu::RcsEngine uncertain; failed.engine=&uncertain;
 assert(uncertain.Initialize(failed.Io(),0x46d18086,0,0x20000,10000));
 failed.failLoad=true;
 assert(!uncertain.Publish(failed.Io(),descriptor));
 assert(uncertain.LastReport().loadAttempted && uncertain.LastReport().error==NativeGpu::RcsEngine::WriteFailed);
 assert(!uncertain.Rearm(failed.Io(),20000));
 Fixture bad; NativeGpu::RcsEngine reject; bad.engine=&reject;
 assert(reject.Initialize(bad.Io(),0x46d18086,0,0x20000,10000));
 assert(!reject.Publish(bad.Io(),descriptor|(3ULL<<61)));
 assert(bad.writes.size()==8);
 Fixture unsafe; unsafe.safe=false; NativeGpu::RcsEngine denied;
 assert(!denied.Initialize(unsafe.Io(),0x46d18086,0,0x20000,10000));
 assert(unsafe.writes.empty());
 Fixture error; error.esr=1; NativeGpu::RcsEngine dirty;
 assert(!dirty.Initialize(error.Io(),0x46d18086,0,0x20000,10000));
 assert(dirty.LastReport().error==NativeGpu::RcsEngine::UnexpectedRegister);
 IntelN100ContextStatus rcs;
 assert(rcs.Arm(1,1,0));
 assert(rcs.Observe(1,Event(0,false))==IntelN100ContextStatus::Accepted);
 assert(rcs.Observe(1,Event(0,true))==IntelN100ContextStatus::Accepted);
 IntelN100ContextStatus wrong;
 assert(wrong.Arm(1,1,0));
 assert(wrong.Observe(1,Event(3,false))==IntelN100ContextStatus::Rejected);
 assert(wrong.LastReport().error==IntelN100ContextStatus::WrongEngine);
 IntelN100ContextStatus bcs;
 assert(bcs.Arm(1,1));
 assert(bcs.Observe(1,0x03ff800000008019ULL)==IntelN100ContextStatus::Accepted);
 assert(bcs.Observe(1,0x0000800003ff8018ULL)==IntelN100ContextStatus::Accepted);
 CsbFixture csb;
 IntelN100CsbReader reader;
 assert(reader.Run(csb.Io(),1,1,10000,8,11,0));
 assert(reader.LastReport().events.state==IntelN100ContextStatus::SwitchedAway);
 CsbFixture bcsCsb; bcsCsb.engineClass=3;
 IntelN100CsbReader rejectBcs;
 assert(!rejectBcs.Run(bcsCsb.Io(),1,1,10000,8,11,0));
 assert(rejectBcs.LastReport().events.error==IntelN100ContextStatus::WrongEngine);
 IntelN100ContextStatus wrongInstance;
 assert(wrongInstance.Arm(1,1,0));
 assert(wrongInstance.Observe(1,Event(0,false)|(1ULL<<6))==IntelN100ContextStatus::Rejected);
 IntelN100ContextStatus unsupported;
 assert(!unsupported.Arm(1,1,2));
 Fixture expired; NativeGpu::RcsEngine bounded; expired.engine=&bounded;
 assert(bounded.Initialize(expired.Io(),0x46d18086,0,0x20000,10000,0));
 assert(bounded.Publish(expired.Io(),descriptor));
 const size_t writesBefore=expired.writes.size();
 assert(!bounded.Rearm(expired.Io(),expired.time+100002));
 assert(bounded.LastReport().error==NativeGpu::RcsEngine::InvalidInput);
 assert(bounded.LastReport().loadAttempted && expired.writes.size()==writesBefore);
 Fixture rollback; NativeGpu::RcsEngine monotonic; rollback.engine=&monotonic;
 assert(monotonic.Initialize(rollback.Io(),0x46d18086,0,0x20000,10000));
 assert(monotonic.Publish(rollback.Io(),descriptor));
 const auto retained=monotonic.LastReport();
 const size_t rollbackWrites=rollback.writes.size();
 rollback.time=0;
 assert(!monotonic.Rearm(rollback.Io(),20000));
 assert(monotonic.LastReport().error==NativeGpu::RcsEngine::Clock);
 assert(monotonic.LastReport().previous==retained.previous);
 assert(monotonic.LastReport().deadline==retained.deadline);
 assert(monotonic.LastReport().writes==retained.writes);
 assert(monotonic.LastReport().loadAttempted==retained.loadAttempted);
 assert(rollback.writes.size()==rollbackWrites);
 Fixture noVisibility; NativeGpu::RcsEngine invisible; noVisibility.engine=&invisible;
 assert(invisible.Initialize(noVisibility.Io(),0x46d18086,0,0x20000,10000));
 noVisibility.visibility=false;
 assert(!invisible.Publish(noVisibility.Io(),descriptor));
 assert(!invisible.LastReport().loadAttempted && noVisibility.writes.size()==8);
 puts("RCS publication/CSB smoke: PASS (fake IO only)");
}
