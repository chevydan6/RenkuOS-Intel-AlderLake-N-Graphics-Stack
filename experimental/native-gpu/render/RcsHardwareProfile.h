// SPDX-License-Identifier: MIT
// Copyright © 2014 Intel Corporation
// Native adaptation Copyright (c) 2026 chevydan6
#ifndef NATIVE_GPU_RCS_HARDWARE_PROFILE_H
#define NATIVE_GPU_RCS_HARDWARE_PROFILE_H
#include <stdint.h>
namespace NativeGpu {
// Pure Linux v6.6 ADL-P_N A0 selection. No IO, ownership or readiness grant.
class RcsHardwareProfile {
public:
 enum Step { A0, D0 };
 enum Kind { Masked16, ReadModifyWrite, WriteOnly, McrMasked16, McrReadModifyWrite };
 enum Scope { Context=1, Engine=2, SharedRender=4, SharedGt=8 };
 struct Operation {
  uint32_t offset, clear, set, readMask;
  Kind kind;
  unsigned scope; // flags: merged engine/shared-render entries retain both.
 };
 struct Snapshot {
  uint32_t identity, revision;
  uint32_t sliceEnable;       // MMIO 0x9138, low 8 bits defined
  uint32_t geometryDssEnable; // MMIO 0x913c; six DSS slots
  uint32_t euDisable;         // MMIO 0x9134, low 8: one fuse per EU pair
  uint32_t vcsMask;           // already discovered/admitted VCS instances 0/2
  uint32_t mirrorFuse3;       // MMIO 0x9118, low 4 L3 bank-disable fuses
 };
 struct Profile {
  uint32_t sliceMask, dssMask, euMask, euCount, dssCount, rpcs, steeringDss;
  uint32_t l3BankMask, l3SteeringBank;
  bool defaultSteeringL3Valid;
  Step graphicsStep, mediaStep, displayStep;
  bool slicePowerGating, dssPowerGating, euPowerGating;
  unsigned contextCount, engineCount, gtCount;
  Operation context[5], engine[9], gt[5];
 };
 static bool Equal(const Operation& a, const Operation& b)
 {
  return a.offset==b.offset && a.clear==b.clear && a.set==b.set
   && a.readMask==b.readMask && a.kind==b.kind && a.scope==b.scope;
 }
 static bool Build(const Snapshot& snapshot, Profile& output)
 {
  const Snapshot s=snapshot;
  // Fail closed on unsupported topology instead of Linux's WARN-and-continue.
  if (s.identity!=0x46d18086 || s.revision!=0 || (s.sliceEnable&255)!=1
   || !s.geometryDssEnable || (s.geometryDssEnable&~63U)
   || (s.euDisable&255)==255 || (s.vcsMask&~5U) || (s.mirrorFuse3&15)==15) return false;
  Profile p{};
  p.sliceMask=s.sliceEnable&255;
  p.dssMask=s.geometryDssEnable;
  for (unsigned i=0;i<8;++i)
   if (!(s.euDisable&(1U<<i))) p.euMask|=3U<<(2*i);
  p.dssCount=Count(p.dssMask);
  p.euCount=Count(p.euMask)*p.dssCount;
  p.slicePowerGating=true;
  // gen12_sseu_info_init: no DSS/EU PG; RPCS uses actual observed slice mask.
  p.rpcs=(1U<<31)|(1U<<18)|(Count(p.sliceMask)<<12);
  while (!(p.dssMask&(1U<<p.steeringDss))) ++p.steeringDss;
  // intel_gt_mcr_init + icl_wa_init_mcr: keep explicit L3 steering when
  // the lowest live DSS does not select a live L3 bank.
  p.l3BankMask=(~s.mirrorFuse3)&15;
  while (!(p.l3BankMask&(1U<<p.l3SteeringBank))) ++p.l3SteeringBank;
  p.defaultSteeringL3Valid=(p.l3BankMask&(1U<<p.steeringDss))!=0;
  p.graphicsStep=A0; p.mediaStep=A0; p.displayStep=D0;
  // gen12_ctx_workarounds_init; sorted as _wa_add. CPU must not apply these.
  p.context[0]={0x2580,0,0x00060002,6,Masked16,Context};
  // Wa_1608008084: FF_MODE2 CPU readback lies; unconditional desired value.
  p.context[1]={0x6604,~0U,0xe0040000,0,WriteOnly,Context};
  p.context[2]={0x7018,0,0x20002000,0x2000,Masked16,Context};
  p.context[3]={0x7300,0,0x00400040,0x40,Masked16,Context};
  p.context[4]={0x7304,0,0x02000200,0x200,Masked16,Context};
  p.contextCount=5;
  // engine_fake_wa_init + general_render_compute_wa_init + rcs_engine_wa_init.
  // CMD_CCTL UC index 3 (gen12_mocs_table): read/write values are index << 1.
  p.engine[0]={0x2050,0,0x10801080,0x1080,Masked16,Engine};
  p.engine[1]={0x20a0,0x80000,0x80000,0x80000,ReadModifyWrite,Engine};
  p.engine[2]={0x20c4,0,0x3fff0306,0x3fff,Masked16,Engine};
  p.engine[3]={0x20e0,0,0x40004000,0x4000,Masked16,Engine};
  p.engine[4]={0x20ec,0,0x00020002,2,Masked16,Engine};
  p.engine[5]={0xb004,0x80,0,0x80,ReadModifyWrite,SharedRender};
  // Linux _wa_add OR-merges disjoint masked sampler fields (bit0 + bit15).
  p.engine[6]={0xe18c,0,0x80018001,0x8001,McrMasked16,Engine|SharedRender};
  p.engine[7]={0xe48c,0,0x02000200,0x200,McrMasked16,Engine};
  // Same merge for early-read bit14 and push-constant hold-disable bit8.
  p.engine[8]={0xe4f4,0,0x41004100,0x4100,McrMasked16,Engine};
  p.engineCount=9;
  // gen12_gt_workarounds_init: gt_tuning_settings has no selected ADL branch.
  // icl_wa_init_mcr: preserve multicast bit31; only replace steering fields.
  p.gt[0]={0xfdc,0x7f000000,p.steeringDss<<24,0x7f000000,ReadModifyWrite,SharedGt};
  // Firmware may lock MISCCPCTL. Linux intentionally skips verification.
  p.gt[1]={0x9424,2,0,0,ReadModifyWrite,SharedGt};
  p.gt[2]={0x9550,0x200,0x200,0x200,McrReadModifyWrite,SharedGt};
  p.gtCount=3;
  // wa_14011060649 affects ALL enabled even video-decode engines, not RCS.
  if (s.vcsMask&1) p.gt[p.gtCount++]={0x1c3f10,0x400000,0x400000,0x400000,ReadModifyWrite,SharedGt};
  if (s.vcsMask&4) p.gt[p.gtCount++]={0x1d3f10,0x400000,0x400000,0x400000,ReadModifyWrite,SharedGt};
  output=p;
  return true;
 }
private:
 static unsigned Count(uint32_t v) { unsigned n=0; for (;v;v&=v-1) ++n; return n; }
};
}
#endif
