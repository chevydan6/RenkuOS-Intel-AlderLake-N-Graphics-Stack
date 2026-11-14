// SPDX-License-Identifier: MIT
// Copyright © 2008-2018 Intel Corporation
// Native adaptation Copyright (c) 2026 chevydan6
#ifndef NATIVE_GPU_RCS_RECOVERY_H
#define NATIVE_GPU_RCS_RECOVERY_H
#include <stdint.h>
namespace NativeGpu {
// Linux v6.6 intel_engine_cs.c::__intel_engine_stop_cs and
// intel_reset.c::gen8_engine_reset_prepare/cancel, gen6_hw_domain_reset.
// Fixed N100 rev0 RCS0 render domain (BIT1) only. Retains Wa_22011802037 and the pre-12.70
// double GDRST + 50us settle. No full-GT reset or forced non-ready retry.
// Offline helper, not complete RCS initialization or permission to reset/free.
// Caller proves exclusive RCS and shared render-domain ownership, all forcewake,
// DMA safety and bounded callbacks for the entire 10ms attempt. Never reset a
// domain shared with an unowned producer. Successful quiescence is evidence only;
// external ownership/retirement rules still govern power release and backing.
// Reuse requires rebuilding context/engine and revalidating DMA/ownership.
class RcsRecovery {
public:
 enum Error : uint32_t { Ok, AlreadyUsed, InvalidIO, Unsafe, ClockError,
  Deadline, ReadError, WriteError, UnexpectedRegister, Timeout };
 enum Stage : uint32_t { Empty, Stop, DisablePrefetch, WaitIdle, PendingWake,
  Prepare, ResetFirst, ResetSecond, Settle, Cancel, Verify, Quiescent };
 struct Report {
  uint32_t error=0, stage=0, reads=0, writes=0, resetRounds=0;
  uint32_t lastOffset=0,lastValue=0,attemptedOffset=0,attemptedValue=0;
  uint32_t cancelAttempted=0,cancelSucceeded=0;
  uint64_t elapsed=0;
 };
 struct IO {
  void* context;
  uint64_t (*now)(void*);
  bool (*safe)(void*);
  bool (*read)(void*,uint32_t,uint32_t&);
  bool (*write)(void*,uint32_t,uint32_t);
  void (*pause)(void*);
 };
 const Report& LastReport() const {return fReport;}
 Error Run(const IO& io) {
  if(fUsed)return AlreadyUsed;
  fUsed=true;
  if(!io.now || !io.safe || !io.read || !io.write || !io.pause)return Fail(InvalidIO);
  fIO=io;fStart=fLast=io.now(io.context);
  if(fStart>UINT64_MAX-10000){fIO={};return Fail(ClockError);}
  bool prepared=false;
  bool success=Sequence(prepared);
  // Linux cancels REQUEST_RESET after both successful and failed attempts.
  // Preserve the original failure while still recording cancellation.
  uint32_t error=fReport.error,stage=fReport.stage;
  if(prepared){
   fReport.cancelAttempted=1;
   bool cancelled=Store(Cancel,0x20d0,0x00010000)
    && Poll(Cancel,0x20d0,5,0,700);
   fReport.cancelSucceeded=cancelled;
   if(!success){fReport.error=error;fReport.stage=stage;}
   success=success && cancelled;
  }
  if(success){
   fReport.stage=Verify;
   success=Read(0x203c) && Require(!(fReport.lastValue&1))
    && Read(0x209c) && Require((fReport.lastValue&0x200)!=0)
    && Read(0x941c) && Require(fReport.lastValue==0) && Check();
   if(success)fReport.stage=Quiescent;
  }
  fIO={};return Error(fReport.error);
 }
private:
 Error Fail(Error error){fReport.error=error;return error;}
 bool Require(bool condition){if(!condition)Fail(UnexpectedRegister);return condition;}
 bool Time(){
  uint64_t now=fIO.now(fIO.context);
  if(now==UINT64_MAX || now<fLast){Fail(ClockError);return false;}
  fLast=now;fReport.elapsed=now-fStart;
  if(fReport.elapsed>=10000){Fail(Deadline);return false;}
  return true;
 }
 bool Check(){
  if(!Time())return false;
  if(!fIO.safe(fIO.context)){Fail(Unsafe);return false;}
  return Time();
 }
 bool Read(uint32_t offset){
  if(!Check())return false;
  ++fReport.reads;fReport.lastOffset=offset;
  if(!fIO.read(fIO.context,offset,fReport.lastValue) || fReport.lastValue==UINT32_MAX){
   Fail(ReadError);return false;
  }return Check();
 }
 bool Store(Stage stage,uint32_t offset,uint32_t value){
  fReport.stage=stage;if(!Check())return false;
  ++fReport.writes;fReport.attemptedOffset=offset;fReport.attemptedValue=value;
  if(!fIO.write(fIO.context,offset,value)){Fail(WriteError);return false;}
  return Check();
 }
 bool Poll(Stage stage,uint32_t offset,uint32_t mask,uint32_t wanted,uint64_t budget){
  fReport.stage=stage;uint64_t end=fLast+budget;
  for(unsigned count=0;count<4096;count++){
   if(!Read(offset))return false;
   if(fLast>=end){Fail(Timeout);return false;}
   if((fReport.lastValue&mask)==wanted)return true;
   fIO.pause(fIO.context);
  }Fail(Timeout);return false;
 }
 bool Sequence(bool& prepared){
  if(!Read(0x941c) || !Require(fReport.lastValue==0)
   || !Store(Stop,0x209c,0x01000100)
   || !Store(DisablePrefetch,0x229c,0x04000400)
   || !Poll(WaitIdle,0x209c,0x200,0x200,1000)
   || !Read(0x209c))return false; // posting read after stop
  fReport.stage=PendingWake;
  if(!Read(0x8000) || !Require(!(fReport.lastValue&(fReport.lastValue>>16)&0x3e00)))return false;
  fReport.stage=Prepare;
  if(!Read(0x20d0))return false;
  uint32_t ack=fReport.lastValue;
  prepared=true; // cancellation also required after an ambiguous store
  if(ack&4){
   // HAS#396813: catastrophic error bypasses the READY handshake.
   if(!Store(Prepare,0x20d0,0x00040004) || !Poll(Prepare,0x20d0,4,0,700))return false;
  }else if(!(ack&2)){
   if(!Store(Prepare,0x20d0,0x00010001) || !Poll(Prepare,0x20d0,2,2,700))return false;
  }
  for(unsigned round=0;round<2;round++){
   Stage stage=round?ResetSecond:ResetFirst;
   if(!Read(0x941c) || !Require(fReport.lastValue==0)
    || !Store(stage,0x941c,2) || !Poll(stage,0x941c,2,0,2000))return false;
   ++fReport.resetRounds;
  }
  fReport.stage=Settle;uint64_t end=fLast+50;
  for(unsigned count=0;count<4096;count++){
   if(!Check())return false;
   if(fLast>=end)return true;
   fIO.pause(fIO.context);
  }Fail(Timeout);return false;
 }
 bool fUsed=false;
 IO fIO={};Report fReport={};
 uint64_t fStart=0,fLast=0;
};
}
#endif
