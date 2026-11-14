// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_OWNERSHIP_H
#define NATIVE_GPU_RCS_OWNERSHIP_H
#include <stdint.h>
namespace NativeGpu {
// CPU admission policy, not hardware exclusion or retirement proof. Caller holds
// the device-wide owner lock for EVERY access, including queries. The resident
// owner retains this object for the boot; reconstructing it is not recovery.
// Only a trusted kernel-private adapter may acknowledge a verified display
// profile. A user flag, idle engine or memory-owner readiness is not a handoff.
class RcsOwnership {
public:
 enum State { Legacy, Granted, Draining, Lost };
 enum Admission { Accepted, Busy, Unavailable };
 // Semantic profile: exact matching-source audit and live driver validation
 // belong to the adapter. This value alone does not authenticate either.
 enum Profile { UnknownProfile, N100DisplayOnlyV1 };

 // Open/reopen generations increase for the entire boot, without wrapping.
 // The display driver must withdraw and drain before a conflicting transition.
 bool DisplayOpened(uint64_t generation) {
  if(fState == Lost || fActive || generation == 0 || generation <= fGeneration)
   return false;
  fGeneration = generation;
  fState = Legacy;
  return true;
 }
 // Acknowledgment means all legacy RCS writers are excluded until withdrawal,
 // including primary/clone init, teardown and hooks. Handoff ids increase across
 // display generations for the boot. No admission is granted by DisplayOpened.
 bool AcknowledgeHandoff(uint64_t generation, uint64_t handoff, Profile profile) {
  if(fState != Legacy || generation == 0 || generation != fGeneration
   || handoff == 0 || handoff <= fHandoff || profile != N100DisplayOnlyV1)
   return false;
  fHandoff = handoff;
  fState = Granted;
  return true;
 }
 Admission Begin(uint64_t generation, uint64_t handoff, uint64_t request) {
  if(fState != Granted || !Matches(generation, handoff))return Unavailable;
  if(fActive)return Busy;
  if(request == 0 || request <= fRequest)return Unavailable;
  fRequest = request;
  fActive = true;
  return Accepted;
 }
 // Close admission synchronously, before the adapter attempts bounded draining.
 // A true return can still leave a backing hold; inspect HoldsBacking before
 // display teardown, allocation release or any legacy writer can resume.
 bool Withdraw(uint64_t generation, uint64_t handoff) {
  if((fState != Granted && fState != Draining) || !Matches(generation, handoff))
   return false;
  fState = fActive ? Draining : Legacy;
  return true;
 }
 // retired means engine-specific completion and visibility were proven outside
 // this class. Uncertain work permanently retains the hold until reboot. No
 // recovery/reset API is provided by this foundation.
 bool Finish(uint64_t generation, uint64_t handoff, uint64_t request,
  bool retired) {
  if((fState != Granted && fState != Draining) || !fActive
   || !Matches(generation, handoff) || request != fRequest)return false;
  if(!retired) {
   fState = Lost;
   return true;
  }
  fActive = false;
  if(fState == Draining)fState = Legacy;
  return true;
 }
 State Current() const { return fState; }
 bool HoldsBacking() const { return fActive; }
private:
 bool Matches(uint64_t generation, uint64_t handoff) const {
  return generation != 0 && generation == fGeneration
   && handoff != 0 && handoff == fHandoff;
 }
 State fState = Legacy;
 bool fActive = false;
 uint64_t fGeneration = 0, fHandoff = 0, fRequest = 0;
};
}
#endif
