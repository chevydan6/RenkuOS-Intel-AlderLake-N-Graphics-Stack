// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_REQUEST_LIFECYCLE_H
#define NATIVE_GPU_REQUEST_LIFECYCLE_H
#include <stdint.h>
namespace NativeGpu {
// Admission policy only. The caller holds its device lock for every access.
// Hardware completion, reset and power validation happen outside this class;
// only their proven result may be supplied to Finish/Resume.
class RequestLifecycle {
public:
 enum State : uint32_t { Cold, Ready, Running, Quiescing, Suspended, Lost, Shutdown };
 enum Admission { Accepted, Busy, Unavailable };
 Admission Begin() {
  if(fState==Running)return Busy;
  if(fState!=Cold && fState!=Ready)return Unavailable;
  fState=Running;return Accepted;
 }
 void Finish(bool retired,bool recovered=false) {
  if(fState!=Running && fState!=Quiescing)return;
  if(retired)++fCompleted;
  else {
   ++fFailed;
   if(!recovered){fState=Lost;return;}
   ++fGeneration;
  }
  fState=fState==Quiescing?fTarget:Ready;
 }
 // Read-only diagnostic work shares admission/power draining without claiming
 // any completed or failed GPU request. Unsafe power/owner state still loses admission.
 void FinishObservation(bool safe) {
  if(fState!=Running && fState!=Quiescing)return;
  if(!safe){fState=Lost;return;}
  fState=fState==Quiescing?fTarget:Ready;
 }
 bool Freeze(State target) {
  if(target!=Suspended && target!=Shutdown)return false;
  if(fState==Lost || fState==Shutdown || fState==Quiescing)return false;
  fTarget=target;
  fState=fState==Running?Quiescing:target;
  return true;
 }
 bool Resume(bool validated) {
  if(fState!=Suspended)return false;
  if(!validated){fState=Lost;return false;}
  ++fGeneration;fState=Ready;return true;
 }
 State Current() const {return fState;}
 uint64_t Completed() const {return fCompleted;}
 uint64_t Failed() const {return fFailed;}
 uint64_t Generation() const {return fGeneration;}
private:
 State fState=Cold, fTarget=Suspended;
 uint64_t fCompleted=0, fFailed=0, fGeneration=1;
};
}
#endif
