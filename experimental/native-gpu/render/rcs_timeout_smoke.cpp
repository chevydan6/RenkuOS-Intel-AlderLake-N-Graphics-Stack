// SPDX-License-Identifier: MIT
#include "RcsCompletionObservation.h"
#include <IntelN100CsbReader.h>
#include <assert.h>
#include <stdio.h>
struct Fixture {
 uint64_t now=0;
 uint32_t entries=0;
 bool hidden=false;
 static uint64_t Now(void* p){return ++((Fixture*)p)->now;}
 static bool Safe(void*){return true;}
 static bool Tail(void* p,uint32_t offset,uint32_t& value){
  assert(offset==kN100CsbWrite);
  value=NativeGpu::RcsCompletionObservation::Tail(((Fixture*)p)->hidden,0xabcd0002,0);
  return true;
 }
 static bool Entry(void* p,uint32_t offset,uint64_t& value){
  assert(offset==kN100CsbBuffer+8 || offset==kN100CsbBuffer+16);
  ++((Fixture*)p)->entries;
  bool away=offset==kN100CsbBuffer+16;
  value=(uint64_t(away?0x7ff:1)<<15)|(uint64_t(away?1:0x7ff)<<47);
  return true;
 }
 static void Pause(void* p){((Fixture*)p)->now+=10;}
 IntelN100CsbReader::Io Io(){return {this,Now,Safe,Tail,Entry,Safe,Pause};}
};
int main()
{
 using O=NativeGpu::RcsCompletionObservation;
 bool armed=true;
 assert(!O::ConsumeTimeout(armed,NATIVE_GPU_RENDER_PROBE,true) && armed);
 assert(!O::ConsumeTimeout(armed,NATIVE_GPU_RENDER_BOOTSTRAP,true) && armed);
 assert(!O::ConsumeTimeout(armed,NATIVE_GPU_RENDER_DRAW,false) && armed);
 Fixture hidden;hidden.hidden=O::ConsumeTimeout(armed,NATIVE_GPU_RENDER_DRAW,true);
 assert(hidden.hidden && !armed);
 IntelN100CsbReader timeout;
 assert(!timeout.Run(hidden.Io(),1,1,50,128,0,0));
 assert(timeout.LastReport().error==IntelN100CsbReader::Deadline);
 assert(timeout.LastReport().elapsed>=50 && hidden.now<100 && !hidden.entries);
 Fixture limited;limited.hidden=true;IntelN100CsbReader polls;
 assert(!polls.Run(limited.Io(),1,1,10000,2,0,0));
 assert(polls.LastReport().error==IntelN100CsbReader::PollLimit && !limited.entries);
 Fixture visible;visible.hidden=O::ConsumeTimeout(armed,NATIVE_GPU_RENDER_DRAW,true);
 assert(!visible.hidden);IntelN100CsbReader completed;
 assert(completed.Run(visible.Io(),1,1,10000,128,0,0));
 assert(completed.LastReport().events.state==IntelN100ContextStatus::SwitchedAway && visible.entries==2);
 puts("RCS administrative completion hiding: PASS (CPU/fake CSB only)");
}
