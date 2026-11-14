// SPDX-License-Identifier: MIT
#include "BcsRecovery.h"
#include <assert.h>
#include <stdio.h>
#include <vector>
struct Backend {
 uint64_t clock=0;
 uint32_t mode=0, reset=0;
 bool stuck=false, safe=true, domainIssued=false;
 std::vector<uint32_t> stores;
 static uint64_t Now(void* p){return ++((Backend*)p)->clock;}
 static bool Safe(void* p){return ((Backend*)p)->safe;}
 static void Pause(void* p){((Backend*)p)->clock+=100;}
 static bool Read(void* p,uint32_t o,uint32_t& v){
  auto& b=*(Backend*)p;
  switch(o){
   case 0x2209c:v=b.mode;break;
   case 0x220d0:v=b.reset;break;
   case 0x941c:v=b.stuck && b.domainIssued?4:0;break;
   case 0x2203c:case 0x800c:v=0;break;
   default:return false;
  }return true;
 }
 static bool Write(void* p,uint32_t o,uint32_t v){
  auto& b=*(Backend*)p;b.stores.push_back(o);b.stores.push_back(v);
  if(o==0x2209c)b.mode=0x300;
  if(o==0x220d0)b.reset=v==0x00010001?3:0;
  if(o==0x941c)b.domainIssued=true;
  return true;
 }
 NativeGpu::BcsRecovery::IO Io(){return {this,Now,Safe,Read,Write,Pause};}
};
int main()
{
 using R=NativeGpu::BcsRecovery;
 Backend b;R recovery;
 assert(recovery.Run(b.Io())==R::Ok);
 const std::vector<uint32_t> expected={0x2209c,0x01000100,0x2229c,0x04000400,
  0x220d0,0x00010001,0x941c,4,0x941c,4,0x220d0,0x00010000};
 assert(b.stores==expected && recovery.LastReport().resetRounds==2);
 assert(recovery.Run(b.Io())==R::AlreadyUsed);
 Backend stuck;stuck.stuck=true;R failed;
 assert(failed.Run(stuck.Io())!=R::Ok);
 assert(failed.LastReport().stage!=R::Quiescent);
 assert(stuck.stores[stuck.stores.size()-2]==0x220d0
  && stuck.stores.back()==0x00010000); // cancel even a failed domain reset
 Backend cat;cat.reset=4;R catastrophic;
 assert(catastrophic.Run(cat.Io())==R::Ok);
 assert(cat.stores[5]==0x00040004);
 Backend unsafe;unsafe.safe=false;R denied;
 assert(denied.Run(unsafe.Io())==R::Unsafe && unsafe.stores.empty());
 puts("Active BCS stop/reset sequence and failed-reset retention: PASS");
}
