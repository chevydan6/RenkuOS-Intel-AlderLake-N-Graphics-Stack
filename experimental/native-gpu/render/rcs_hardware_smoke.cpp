// SPDX-License-Identifier: MIT
#include "RcsHardwareProfile.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
using NativeGpu::RcsHardwareProfile;
using P = RcsHardwareProfile;
static P::Snapshot valid() { return {0x46d18086, 0, 1, 0x15, 0x92, 5, 0xc}; }
static void reject(P::Snapshot s)
{
 P::Profile p; memset(&p, 0x5a, sizeof p);
 unsigned char original[sizeof p]; memcpy(original, &p, sizeof p);
 assert(!P::Build(s, p)); assert(!memcmp(original, &p, sizeof p));
}
int main()
{
 P::Profile p{};
 assert(P::Build(valid(), p));
 assert(p.sliceMask == 1 && p.dssMask == 0x15 && p.euMask == 0x3cf3);
 assert(p.euCount == 30 && p.dssCount == 3 && p.rpcs == 0x80041000);
 assert(p.steeringDss == 0 && p.graphicsStep == P::A0 && p.mediaStep == P::A0
  && p.displayStep == P::D0 && p.slicePowerGating && !p.dssPowerGating && !p.euPowerGating);
 assert(p.l3BankMask==3 && p.l3SteeringBank==0 && p.defaultSteeringL3Valid);
 assert(p.contextCount == 5 && p.engineCount == 9 && p.gtCount == 5);
 const P::Operation context[] = {
  {0x2580,0,0x00060002,6,P::Masked16,P::Context},
  {0x6604,~0U,0xe0040000,0,P::WriteOnly,P::Context},
  {0x7018,0,0x20002000,0x2000,P::Masked16,P::Context},
  {0x7300,0,0x00400040,0x40,P::Masked16,P::Context},
  {0x7304,0,0x02000200,0x200,P::Masked16,P::Context}};
 for (unsigned i=0; i<5; ++i) assert(P::Equal(p.context[i], context[i]));
 const P::Operation engine[] = {
  {0x2050,0,0x10801080,0x1080,P::Masked16,P::Engine},
  {0x20a0,0x80000,0x80000,0x80000,P::ReadModifyWrite,P::Engine},
  {0x20c4,0,0x3fff0306,0x3fff,P::Masked16,P::Engine},
  {0x20e0,0,0x40004000,0x4000,P::Masked16,P::Engine},
  {0x20ec,0,0x00020002,2,P::Masked16,P::Engine},
  {0xb004,0x80,0,0x80,P::ReadModifyWrite,P::SharedRender},
  {0xe18c,0,0x80018001,0x8001,P::McrMasked16,P::Engine|P::SharedRender},
  {0xe48c,0,0x02000200,0x200,P::McrMasked16,P::Engine},
  {0xe4f4,0,0x41004100,0x4100,P::McrMasked16,P::Engine}};
 for (unsigned i=0; i<9; ++i) assert(P::Equal(p.engine[i], engine[i]));
 const P::Operation gt[] = {
  {0xfdc,0x7f000000,0,0x7f000000,P::ReadModifyWrite,P::SharedGt},
  {0x9424,2,0,0,P::ReadModifyWrite,P::SharedGt},
  {0x9550,0x200,0x200,0x200,P::McrReadModifyWrite,P::SharedGt},
  {0x1c3f10,0x400000,0x400000,0x400000,P::ReadModifyWrite,P::SharedGt},
  {0x1d3f10,0x400000,0x400000,0x400000,P::ReadModifyWrite,P::SharedGt}};
 for (unsigned i=0; i<5; ++i) assert(P::Equal(p.gt[i], gt[i]));
 // Sparse highest DSS selects the lowest live instance; one enabled EU pair.
 auto sparse=valid(); sparse.geometryDssEnable=0x28; sparse.euDisable=0xfe; sparse.vcsMask=0; sparse.mirrorFuse3=0xe;
 assert(P::Build(sparse,p));
 assert(p.dssCount==2 && p.euMask==3 && p.euCount==4 && p.rpcs==0x80041000);
 assert(p.l3BankMask==1 && !p.defaultSteeringL3Valid && p.l3SteeringBank==0);
 assert(p.steeringDss==3 && p.gt[0].set==0x03000000 && p.gtCount==3);
 auto full=valid(); full.geometryDssEnable=0x3f; full.euDisable=0; full.vcsMask=1;
 assert(P::Build(full,p));
 assert(p.euMask==0xffff && p.euCount==96 && p.gtCount==4);
 assert(p.gt[3].offset==0x1c3f10);
 auto s=valid(); s.identity++; reject(s); s=valid(); s.revision=1; reject(s);
 s=valid(); s.sliceEnable=0; reject(s); s.sliceEnable=3; reject(s);
 s=valid(); s.geometryDssEnable=0; reject(s); s.geometryDssEnable=64; reject(s);
 s=valid(); s.euDisable=255; reject(s); s=valid(); s.vcsMask=2; reject(s);
 s=valid(); s.mirrorFuse3=0xf; reject(s);
 puts("rcs_hardware_smoke: PASS");
}
