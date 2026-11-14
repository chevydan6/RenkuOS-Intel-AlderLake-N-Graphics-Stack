// SPDX-License-Identifier: MIT
#include "RcsRecovery.h"
#include <assert.h>
#include <stdio.h>
#include <vector>
struct Backend {
 uint64_t clock=0, lastResetTime=0, cancelTime=0;
 uint32_t mode=0, reset=0, pending=0;
 bool stuckDomain=false, stuckPrepare=false, failPrepare=false, safe=true;
 bool domainIssued=false;
 std::vector<uint32_t> stores, reads;
 static uint64_t Now(void* p){return ++static_cast<Backend*>(p)->clock;}
 static bool Safe(void* p){return static_cast<Backend*>(p)->safe;}
 static void Pause(void* p){static_cast<Backend*>(p)->clock+=100;}
 static bool Read(void* p,uint32_t offset,uint32_t& value){
  auto& b=*static_cast<Backend*>(p); b.reads.push_back(offset);
  switch(offset){
   case 0x209c:value=b.mode;break;
   case 0x20d0:value=b.reset;break;
   case 0x941c:value=b.stuckDomain && b.domainIssued?2:0;break;
   case 0x203c:value=0;break;
   case 0x8000:value=b.pending;break;
   default:assert(false);return false; // In particular, no BCS register access.
  }return true;
 }
 static bool Write(void* p,uint32_t offset,uint32_t value){
  auto& b=*static_cast<Backend*>(p);
  b.stores.push_back(offset);b.stores.push_back(value);
  switch(offset){
   case 0x209c:assert(value==0x01000100);b.mode=0x300;break;
   case 0x229c:assert(value==0x04000400);break;
   case 0x20d0:
    if(value==0x00010001){b.reset=b.stuckPrepare?1:3;return !b.failPrepare;}
    if(value==0x00040004){b.reset=0;break;}
    assert(value==0x00010000);b.cancelTime=b.clock;b.reset=0;break;
   case 0x941c:assert(value==2);b.domainIssued=true;b.lastResetTime=b.clock;break;
   default:assert(false);return false;
  }return true;
 }
 NativeGpu::RcsRecovery::IO Io(){return {this,Now,Safe,Read,Write,Pause};}
};
int main(){
 using R=NativeGpu::RcsRecovery;
 Backend b;R recovery;
 assert(recovery.Run(b.Io())==R::Ok);
 const std::vector<uint32_t> expected={0x209c,0x01000100,0x229c,0x04000400,
  0x20d0,0x00010001,0x941c,2,0x941c,2,0x20d0,0x00010000};
 assert(b.stores==expected && recovery.LastReport().resetRounds==2);
 assert(recovery.LastReport().stage==R::Quiescent);
 assert(b.cancelTime>=b.lastResetTime+50);
 assert(recovery.LastReport().cancelAttempted && recovery.LastReport().cancelSucceeded);
 assert(b.reads[b.reads.size()-3]==0x203c);
 assert(b.reads[b.reads.size()-2]==0x209c && b.reads.back()==0x941c);
 assert(recovery.Run(b.Io())==R::AlreadyUsed);
 Backend stuck;stuck.stuckDomain=true;R failed;
 assert(failed.Run(stuck.Io())==R::Timeout);
 assert(failed.LastReport().stage!=R::Quiescent);
 assert(failed.LastReport().cancelAttempted && failed.LastReport().cancelSucceeded);
 assert(stuck.stores[stuck.stores.size()-2]==0x20d0 && stuck.stores.back()==0x00010000);
 Backend prepare;prepare.stuckPrepare=true;R notReady;
 assert(notReady.Run(prepare.Io())==R::Timeout);
 assert(!prepare.domainIssued && notReady.LastReport().cancelSucceeded);
 Backend ambiguous;ambiguous.failPrepare=true;R writeFailed;
 assert(writeFailed.Run(ambiguous.Io())==R::WriteError);
 assert(!ambiguous.domainIssued && writeFailed.LastReport().cancelSucceeded);
 assert(writeFailed.LastReport().stage!=R::Quiescent);
 Backend cat;cat.reset=4;R catastrophic;
 assert(catastrophic.Run(cat.Io())==R::Ok && cat.stores[5]==0x00040004);
 Backend ready;ready.reset=2;R prepared;
 assert(prepared.Run(ready.Io())==R::Ok && ready.stores[4]==0x941c);
 Backend unsafe;unsafe.safe=false;R denied;
 assert(denied.Run(unsafe.Io())==R::Unsafe && unsafe.stores.empty());
 Backend pending;pending.pending=0x02000200;R wake;
 assert(wake.Run(pending.Io())==R::UnexpectedRegister);
 assert(!pending.domainIssued && wake.LastReport().stage!=R::Quiescent);
 puts("RCS active stop/reset/cancel smoke: PASS (fake IO only)");
}
