// SPDX-License-Identifier: MIT
// Included in the permanent owner after NativeExecution.inc's shared adapters.
#include "RenderModule.h"
#include "render/RcsHardwareProfile.h"
#include "render/RcsDrawImage64.h"
#include "render/RcsOwnership.h"
#include "render/RcsContextImage.h"
#include "render/RcsEngine.h"
#include "render/RcsRecovery.h"
#include "render/RcsRequestReuse.h"
#include "render/RcsUserVertices.h"
#include "render/RcsResourceVertices.h"
#include "render/RcsTwoDrawBatch.h"
#include "render/RcsIndexedBatch.h"
#include "render/RcsValidatedBatch.h"
#include "render/RcsPublicShader.h"
#include "render/RcsUniformShader.h"
#include "render/RcsUniformSurface.h"
#include "render/RcsCompletionObservation.h"
#include "render/RcsPciAdmission.h"
#include "NativeResourceArena.h"
#include "render/RcsResourceTarget.h"
#include "render/RcsResourceRange.h"
#include "ResourceModule.h"
static native_gpu_render_report sRenderReport;
// This grant is reachable only through the private graphics-driver module ABI.
static NativeGpu::RcsOwnership sRenderOwnership;
static uint64_t sRenderDisplayLease,sRenderHandoff,sRenderOperationSerial,sRenderActiveOperation;
static bool NativeRenderOwnerLocked(bool active=false)
{
 if(!sRenderDisplayLease || (sRenderOwnership.Current()!=NativeGpu::RcsOwnership::Granted
   && !(active && sRenderOwnership.Current()==NativeGpu::RcsOwnership::Draining
    && sRenderOwnership.HoldsBacking()))
  || !sState.DiagnosticAllowed())return false;
 IntelApertureLeaseState::Operation ticket={};
 if(sState.BeginOperation(sRenderDisplayLease,IntelApertureLeaseState::Query,ticket)
  !=IntelApertureLeaseState::Success)return false;
 sState.EndOperation(ticket,false);return true;
}
static status_t NativeRcsAcknowledgeDisplay(uint64_t lease,uint32_t profile)
{
 mutex_lock(&sLock);
 status_t result=B_NOT_ALLOWED;
 IntelApertureLeaseState::Operation ticket={};
 if(profile==NativeGpu::RcsOwnership::N100DisplayOnlyV1 && lease && lease!=sMappingLease
  && sRenderHandoff!=UINT64_MAX && sState.DiagnosticAllowed()
  && sState.BeginOperation(lease,IntelApertureLeaseState::Query,ticket)==IntelApertureLeaseState::Success){
  sState.EndOperation(ticket,false);
  if(sRenderOwnership.DisplayOpened(lease)
   && sRenderOwnership.AcknowledgeHandoff(lease,sRenderHandoff+1,NativeGpu::RcsOwnership::N100DisplayOnlyV1)){
   sRenderDisplayLease=lease;++sRenderHandoff;result=B_OK;
  }
 }
 mutex_unlock(&sLock);return result;
}
static status_t NativeRcsDisplayClosing(uint64_t lease)
{
 mutex_lock(&sLock);
 if(lease!=sRenderDisplayLease){mutex_unlock(&sLock);return B_OK;}
 bool closed=sRenderOwnership.Withdraw(lease,sRenderHandoff);
 sNativeLifecycle.Freeze(NativeGpu::RequestLifecycle::Shutdown);
 bool held=sRenderOwnership.HoldsBacking();
 mutex_unlock(&sLock);
 // The synchronous request owns all hardware transitions. Close admission
 // before waiting; the active Draining token may finish safely.
 if(!closed)return B_DEV_NOT_READY;
 if(!held)return B_OK;
 bigtime_t start=system_time();
 while(system_time()-start<250000){
  mutex_lock(&sLock);held=sRenderOwnership.HoldsBacking();mutex_unlock(&sLock);
  if(!held)return B_OK;
  snooze(1000);
 }
 mutex_lock(&sLock);
 if(sRenderOwnership.HoldsBacking()){
  sRenderOwnership.Finish(sRenderDisplayLease,sRenderHandoff,sRenderActiveOperation,false);
  sNativeLifecycle.Finish(false);sObservationBlocked=true;
 }
 mutex_unlock(&sLock);return B_TIMED_OUT;
}
static intel_memory_mapping_buffer sRenderBuffers[4];
static addr_t sRenderBacking[4];
static bool sRenderReady,sRenderPageHold,sRenderPublished,sRenderDefaultValid,sRenderRetired;
static uint32_t sRenderCursor;
static bool sRenderHideCompletion;
static uint32_t* sRenderData[NativeGpu::RcsDrawImage64::PageCount];
static bool sRenderUserDraw;
static bool sRenderLargeDraw;
static uint32_t sRenderExpectedPixel=NativeGpu::RcsDrawImage::RenderedPixel;
static NativeGpu::RcsPublicShader::FragmentProgram sRenderFragmentProgram=
 NativeGpu::RcsPublicShader::ConstantColor;
static unsigned sRenderGradientDenominator;
static bool sRenderGradientGreen;
static bool sRenderGradientCaptured;
static bool sRenderResourceDraw,sRenderArenaDraw,sRenderPublicBatchDraw,sRenderPublicShaderDraw;
static bool sRenderCombinedDraw,sRenderPublicVertexDraw,sRenderResourceReady;
static bool sRenderDirectTarget;
static bool sRenderPreserveTarget;
static bool sRenderTwoDraw;
static bool sRenderIndexedDraw;
static bool sRenderIndexedPair;
static bool sRenderIndexedMesh;
static unsigned sRenderIndexCount,sRenderVertexBytes;
static bool sRenderUniformDraw;
static uint32_t sRenderPublicVertexCode[1024];
static uint64_t sRenderArenaGpu,sRenderPublicBatchGpu,sRenderPublicShaderGpu;
static uint64_t sRenderTargetGpu;
static uint64_t sRenderUniformGpu;
static uint32_t* sRenderTargetPages[NativeGpu::RcsDrawImage64::TargetPages];
static uint32_t sRenderPreviousTarget[4096];
static IntelN100PpgttTree::Page sRenderResourcePage;
static constexpr uint64_t kRenderArenaGpu=NativeGpu::RcsResourceRange::Base;
static constexpr unsigned kRenderArenaPages=NativeGpu::RcsResourceRange::Pages;
static IntelN100PpgttTree::Page sRenderArenaPages[kRenderArenaPages];
static NativeGpu::ResourceArena sRenderArena(kRenderArenaGpu,kRenderArenaPages);
static bool sRenderArenaReady;
static bool NativeResourceAllowedLocked();
static uint32_t sRenderUserVertices[12];
static uint32_t sRenderExpectedVertices[12];
static uint32_t sRenderSecondVertices[12];
static uint16_t sRenderIndices[255];
static uint32_t sRenderSavedDefault[14*1024];
static IntelN100PpgttTree sRenderTree;
static NativeGpu::RcsEngine sRenderEngine;
static IntelN100CsbReader sRenderCsb;
static NativeGpu::RcsHardwareProfile::Profile sRenderProfile;
#ifdef NATIVE_RENDER_PROFILE_TIMING
// One active owner request at a time. This diagnostic is absent from normal builds.
static struct {
 bigtime_t accepted,forcewakeStart,forcewakeDone,workStart,published;
 bigtime_t csbDone,verified,groupDone,finished;
} sRenderTiming;
#endif
static void NativeRenderSnapshotLocked(native_gpu_render_report& r)
{
 r.state_flags=(sNativeRenderInitialized?NATIVE_GPU_RENDER_INITIALIZED:0)
  |(sRenderDefaultValid?NATIVE_GPU_RENDER_DEFAULT_VALID:0)
  |(sRenderReady?NATIVE_GPU_RENDER_POOL_READY:0);
 r.owner_granted=NativeRenderOwnerLocked();r.csb_cursor=sRenderCursor;
}
static bool NativeRenderHold(void*){if(sRenderPageHold)return false;sRenderPageHold=true;return true;}
static void NativeRenderDrop(void*){sRenderPageHold=false;}
static bool NativeRenderAllocate()
{
 if(sRenderReady)return true;
 aperture_info aperture={};
 if(Query(sMappingLease,&aperture)!=B_OK)return false;
 const size_t sizes[]={4096,4096,4096,65536};
 for(unsigned i=0;i<4;i++){
  addr_t base=0;
  if(Allocate(sMappingLease,sizes[i],4096,B_APERTURE_NON_RESERVED,&base,NULL)!=B_OK)return false;
  sRenderBacking[i]=base;
  if(base<aperture.base || (base&4095) || sizes[i]>aperture.size
   || base-aperture.base<aperture.reserved_size
   || base-aperture.base>aperture.size-sizes[i])return false;
  auto& b=sRenderBuffers[i];b.size=sizes[i];b.ggtt_offset=base-aperture.base;
  if(b.ggtt_offset>(1ULL<<32)-sizes[i] || sMappingAliasBase>UINTPTR_MAX-b.ggtt_offset-sizes[i])return false;
  b.cpu_aperture=sMappingAliasBase+b.ggtt_offset;
  for(unsigned j=0;j<4;j++){
   const auto& c=sMappingSet.buffers[j];
   if(b.ggtt_offset<c.ggtt_offset+c.size && c.ggtt_offset<b.ggtt_offset+b.size)return false;
  }
  for(unsigned j=0;j<i;j++){
   const auto& c=sRenderBuffers[j];
   if(b.ggtt_offset<c.ggtt_offset+c.size && c.ggtt_offset<b.ggtt_offset+b.size)return false;
  }
 }
 IntelN100PpgttTree::Backend backend={NULL,NativeRenderHold,NativeRenderDrop,NativeAllocate,NativeFree};
 if(!sRenderTree.Build(backend,0x400000,NativeGpu::RcsDrawImage64::PageCount,
   3,3,IntelN100DirectoryUncached))return false;
 for(unsigned i=0;i<NativeGpu::RcsDrawImage64::PageCount;i++){
  IntelN100PpgttTree::Page page={};if(!sRenderTree.Describe(8+i,page))return false;
  sRenderData[i]=(uint32_t*)page.cpu;
 }
 // This distinct vertex page is optional for the v1/v2 controls. A failed
 // allocation or bind is retained and disables only v3; it cannot alter a
 // published PTE or invalidate the known-good fixed workloads.
 if(NativeAllocate(NULL,sRenderResourcePage)
  && sRenderTree.BindPages(NativeGpu::RcsResourceVertices::VertexGpu,&sRenderResourcePage,1,3))
  sRenderResourceReady=true;
 bool arenaAllocated=true;
 unsigned arenaCount=0;
 for(;arenaCount<kRenderArenaPages;arenaCount++)
  if(!NativeAllocate(NULL,sRenderArenaPages[arenaCount])){arenaAllocated=false;break;}
 bool arenaBound=arenaAllocated
  && sRenderTree.BindPages(kRenderArenaGpu,sRenderArenaPages,kRenderArenaPages,3);
 if(arenaBound)
 {
  // Kernel add-ons do not reliably run C++ global constructors. Construct
  // this nonzero-initialized ledger explicitly before any resource session.
  new (&sRenderArena) NativeGpu::ResourceArena(kRenderArenaGpu,kRenderArenaPages);
  sRenderArenaReady=true;
 }
 else dprintf("native RCS resource pool unavailable allocated=%u bind_attempted=%u\n",
  arenaCount,uint32_t(arenaAllocated));
 sRenderReady=true;return true;
}
static bool NativeRenderOffset(uint32_t offset)
{
 if(offset>=0x2000 && offset<=0x25ac && !(offset&3))return true;
 if((offset>=0x4000 && offset<=0x40fc) || (offset>=0x4800 && offset<=0x481c)
  || (offset>=0xb020 && offset<=0xb09c))return !(offset&3);
 const uint32_t allowed[]={0xfdc,0x8000,0x941c,0x9424,0x9550,0xb004,
  0xe18c,0xe48c,0xe4f4,0x1c3f10,0x1d3f10,0xced8,0xceec,0xcec4};
 for(unsigned i=0;i<sizeof(allowed)/sizeof(allowed[0]);i++)if(offset==allowed[i])return true;
 return false;
}
static bool NativeRenderGroupSafe(void* data)
{
 if(!NativeGroupSafe(data))return false;
 if(sRenderReport.mode==NATIVE_GPU_RENDER_PROBE)return true;
 mutex_lock(&sLock);bool owner=NativeRenderOwnerLocked(true);mutex_unlock(&sLock);
 return owner && NativeDmaStillDirect();
}
static bool NativeRenderAccessSafe(void* data)
{
 if(gKernelShutdown || !ResetSafe(data) || !NativeDmaStillDirect())return false;
 mutex_lock(&sLock);bool owner=NativeRenderOwnerLocked(true);mutex_unlock(&sLock);
 return owner;
}
static bool NativeRenderRawRead(void* data,uint32_t offset,uint32_t& value)
{
 auto& c=*(GroupContext*)data;
 if(!NativeRenderAccessSafe(data) || !NativeRenderOffset(offset) || uint64(offset)+4>c.handshake->device.size
  || c.handshake->registers>UINTPTR_MAX-offset-3)return false;
 memory_read_barrier();value=*(const volatile uint32*)(c.handshake->registers+offset);memory_read_barrier();
 return value!=UINT32_MAX && NativeRenderAccessSafe(data);
}
static bool NativeRenderRawWrite(void* data,uint32_t offset,uint32_t value)
{
 auto& c=*(GroupContext*)data;
 if(!NativeRenderAccessSafe(data) || !NativeRenderOffset(offset) || offset==0xcec4 || uint64(offset)+4>c.handshake->device.size
  || c.handshake->registers>UINTPTR_MAX-offset-3)return false;
 memory_write_barrier();*(volatile uint32*)(c.handshake->registers+offset)=value;memory_full_barrier();return NativeRenderAccessSafe(data);
}
static bool NativeRenderResetSafe(void* data)
{
 if(gKernelShutdown || !ResetSafe(data) || !NativeDmaStillDirect())return false;
 mutex_lock(&sLock);bool owner=NativeRenderOwnerLocked(true);mutex_unlock(&sLock);
 uint32_t fault=0;
 return owner && NativeRenderRawRead(data,0xcec4,fault) && fault==0;
}
static bool NativeRenderSafe(void* data)
{
 uint32_t eir=0,esr=0;
 if(!NativeRenderResetSafe(data) || !NativeRenderRawRead(data,0x20b0,eir)
  || !NativeRenderRawRead(data,0x20b8,esr))return false;
 sRenderReport.eir=eir;sRenderReport.esr=esr;return eir==0 && esr==0;
}
static bool NativeRenderStore(void* data,uint32_t offset,uint32_t value)
{return NativeRenderSafe(data) && NativeRenderRawWrite(data,offset,value) && NativeRenderSafe(data);}
static bool NativeRenderApply(void* data,const NativeGpu::RcsHardwareProfile::Operation& op)
{
 using P=NativeGpu::RcsHardwareProfile;
 if(op.scope&P::Context)return false;
 bool mcr=op.kind==P::McrMasked16 || op.kind==P::McrReadModifyWrite;
 uint32_t oldSelector=0;
 if(mcr){
  if(!NativeRenderRawRead(data,0xfdc,oldSelector) || !(oldSelector&0x80000000))return false;
  // ADL-N default DSS steering; the selected MCR offsets are outside the
  // special L3BANK range 0xb100..0xb3ff (intel_gt_mcr.c icl table).
  uint32_t selector=(oldSelector&~0x7f000000U)|(sRenderProfile.steeringDss<<24);
  if(!NativeRenderStore(data,0xfdc,selector))return false;
 }
 uint32_t value=op.set;
 bool success=true;
 if(op.kind==P::ReadModifyWrite || op.kind==P::McrReadModifyWrite){
  success=NativeRenderSafe(data) && NativeRenderRawRead(data,op.offset,value);
  value=(value&~op.clear)|op.set;
 }
 if(success)success=NativeRenderStore(data,op.offset,value);
 if(success && op.readMask){
  uint32_t observed=0;
  success=NativeRenderRawRead(data,op.offset,observed) && (observed&op.readMask)==(value&op.readMask);
 }
 if(mcr){bool restored=NativeRenderStore(data,0xfdc,oldSelector);success=success && restored;}
 return success;
}
static bool NativeRenderStatus32(void*,uint32_t offset,uint32_t& value)
{
 if(offset!=kN100CsbWrite)return false;
 uint32_t raw=*(const volatile uint32_t*)(sRenderBuffers[1].cpu_aperture+offset);
 value=NativeGpu::RcsCompletionObservation::Tail(sRenderHideCompletion,raw,sRenderCursor);return true;
}
static bool NativeRenderStatus64(void*,uint32_t offset,uint64_t& value)
{
 if(offset<kN100CsbBuffer || offset>=kN100CsbBuffer+12*8 || (offset&7))return false;
 value=*(const volatile uint64_t*)(sRenderBuffers[1].cpu_aperture+offset);return true;
}
static bool NativeRenderRead(void* data,uint32_t offset,uint32_t& value)
{
 auto& context=*(GroupContext*)data;
 if(sRenderReport.mode!=NATIVE_GPU_RENDER_PROBE){
  mutex_lock(&sLock);bool owner=NativeRenderOwnerLocked(true);mutex_unlock(&sLock);
  if(!owner)return false;
 }
 bool allowed=offset==0x9138 || offset==0x913c || offset==0x9134
  || offset==0x9140 || offset==0x9118 || offset==0x203c || offset==0x209c
  || offset==0x2234 || offset==0x2238 || offset==0x20b0 || offset==0x20b8
  || offset==0xcec4;
 if(!allowed || uint64(offset)+4>context.handshake->device.size
  || context.handshake->registers>UINTPTR_MAX-offset-3
  || gKernelShutdown || !ResetSafe(data) || !NativeDmaStillDirect())return false;
 memory_read_barrier();
 value=*(const volatile uint32*)(context.handshake->registers+offset);
 memory_read_barrier();
 return value!=UINT32_MAX && ResetSafe(data) && NativeDmaStillDirect();
}
static bool NativeRenderProbe(void* data,uint64_t deadline)
{
 auto& context=*(GroupContext*)data;
 context.resetDeadline=deadline;context.resetLastTime=HandshakeTime(NULL);
 auto& r=sRenderReport;r.stage=4;
 if(!NativeRenderRead(data,0x9138,r.slice_enable)
  || !NativeRenderRead(data,0x913c,r.geometry_dss_enable)
  || !NativeRenderRead(data,0x9134,r.eu_disable)
  || !NativeRenderRead(data,0x9140,r.media_disable)
  || !NativeRenderRead(data,0x9118,r.mirror_fuse3)
  || !NativeRenderRead(data,0x203c,r.ring_control)
  || !NativeRenderRead(data,0x209c,r.ring_mode)
  || !NativeRenderRead(data,0x2234,r.el_status_lo)
  || !NativeRenderRead(data,0x2238,r.el_status_hi)
  || !NativeRenderRead(data,0x20b0,r.eir)
  || !NativeRenderRead(data,0x20b8,r.esr)
  || !NativeRenderRead(data,0xcec4,r.fault))return false;
 NativeGpu::RcsHardwareProfile::Snapshot snapshot={r.identity,r.revision,
  r.slice_enable,r.geometry_dss_enable,r.eu_disable,(~r.media_disable)&5,r.mirror_fuse3};
 auto& profile=sRenderProfile;profile={};
 r.known_profile=NativeGpu::RcsHardwareProfile::Build(snapshot,profile);
 if(r.known_profile){r.slice_mask=profile.sliceMask;r.dss_mask=profile.dssMask;
  r.eu_mask=profile.euMask;r.eu_count=profile.euCount;r.rpcs=profile.rpcs;}
 r.stage=5;
 // Read-only probing never clears error registers. A shared fault is uncertain.
 return r.fault==0 && NativeOwnerPower(context) && NativeDmaStillDirect();
}
// Linux gen12_emit_flush_rcs, intel_engine_emit_ctx_wa,
// gen12_emit_fini_breadcrumb_rcs/tail. Polling replaces MI_USER_INTERRUPT by NOOP.
static bool NativeRenderBuild(uint32_t nonce,uint32_t sequence,uint32_t& tail,bool draw)
{
 uint32_t* ring=(uint32_t*)sRenderBuffers[0].cpu_aperture;
 uint32_t* status=(uint32_t*)sRenderBuffers[1].cpu_aperture;
 memset(ring,0,4096);memset((void*)sRenderBuffers[2].cpu_aperture,0x5a,4096);
 if((!draw && !intel_n100_initialize_hwsp(status,4096))
  || !intel_n100_ggtt_nonce_stores(sRenderBuffers[2].ggtt_offset,nonce,ring,1024))return false;
 if(!draw){sRenderCursor=0;status[kN100CsbWrite/4]=0;}
 if(!NativeGpu::RcsRequestReuse::PoisonNext(status,sRenderCursor))return false;
 status[0x40]=NativeGpu::kPoison;status[0xc8/4]=0;
 unsigned n=8;
 auto pc=[&](uint32_t flags0,uint32_t flags1,uint32_t address,uint32_t value){
  ring[n++]=0x7a000004|flags0;ring[n++]=flags1;ring[n++]=address;
  ring[n++]=0;ring[n++]=value;ring[n++]=0;
 };
 auto barrier=[&](){
  // HDC + tile/L3/RT/depth/DC flush, Wa_1409600907 depth stall; ppHWSP scratch.
  pc(1U<<9,(1U<<28)|(1U<<27)|(1U<<21)|(1U<<20)|(1U<<14)|(1U<<13)|(1U<<12)|(1U<<7)|(1U<<5)|1,0xd0,0);
  ring[n++]=0x02800101; // preparser_disable(true)
  pc(0,(1U<<29)|(1U<<21)|(1U<<20)|(1U<<18)|(1U<<14)|(1U<<11)|(1U<<10)|(1U<<4)|(1U<<3)|(1U<<2),0xd0,0);
  const uint32_t aux[]={0x11020001,0x4208,1,0x0e01c003,0,0x4208,0,0};
  memcpy(ring+n,aux,sizeof(aux));n+=8;
  ring[n++]=0x02800100; // preparser_disable(false)
 };
 barrier();
 if(draw){NativeGpu::RcsRequestReuse::BatchStart(ring+n,
  sRenderPublicBatchDraw?uint32_t(sRenderPublicBatchGpu):0x400000);n+=6;}
 else {
  ring[n++]=0x11000000|(2*sRenderProfile.contextCount-1);
  for(unsigned i=0;i<sRenderProfile.contextCount;i++){
   ring[n++]=sRenderProfile.context[i].offset;ring[n++]=sRenderProfile.context[i].set;
  }
  ring[n++]=0;barrier();
 }
 pc(1U<<9,(1U<<28)|(1U<<27)|(1U<<20)|(1U<<18)|(1U<<13)|(1U<<12)|(1U<<7)|(1U<<5)|1,0,0);
 pc(0,(1U<<24)|(1U<<20)|(1U<<14)|(1U<<7),uint32_t(sRenderBuffers[1].ggtt_offset+0x100),sequence);
 ring[n++]=0;ring[n++]=0x04000001;
 ring[n++]=0x02800000;ring[n++]=0x0e40c003;ring[n++]=0;
 ring[n++]=uint32_t(sRenderBuffers[1].ggtt_offset+0xc8);ring[n++]=0;ring[n++]=0;
 tail=n*4;ring[n++]=0x02800000;ring[n++]=0;
 uint64_t root=0;if(!sRenderTree.Root(root))return false;
 if(draw)return NativeGpu::RcsRequestReuse::Restore((uint32_t*)sRenderBuffers[3].cpu_aperture,
  sRenderSavedDefault,sRenderBuffers[0].ggtt_offset,root,tail,sRenderProfile.rpcs);
 NativeGpu::RcsContextImage::Input in={sRenderReport.identity,sRenderReport.revision,
  sRenderBuffers[3].ggtt_offset,sRenderBuffers[0].ggtt_offset,root,4096,tail,sRenderProfile.rpcs};
 return NativeGpu::RcsContextImage::Build(in,(uint32_t*)sRenderBuffers[3].cpu_aperture,65536);
}
static bool NativeRenderRequest(void* data,uint64_t deadline)
{
#ifdef NATIVE_RENDER_PROFILE_TIMING
 sRenderTiming.workStart=system_time();
#endif
 if(!NativeRenderProbe(data,deadline) || !sRenderReport.known_profile)return false;
 sNativeRenderActive=true;auto& r=sRenderReport;
 bool draw=r.mode==NATIVE_GPU_RENDER_DRAW;
 if(draw){
  if(!sRenderRetired || !sRenderDefaultValid || !sNativeRenderInitialized
   || !NativeRenderSafe(data))return false;
 }else {
 r.stage=7;
 NativeGpu::RcsRecovery reset;
 NativeGpu::RcsRecovery::IO resetIO={data,HandshakeTime,NativeRenderResetSafe,
  NativeRenderRawRead,NativeRenderRawWrite,HandshakePause};
 bool resetOk=reset.Run(resetIO)==NativeGpu::RcsRecovery::Ok;
 r.reset_stage=reset.LastReport().stage;r.reset_error=reset.LastReport().error;
 if(!resetOk || !NativeRenderSafe(data))return false;
 // Every GT/engine operation is selected by the fixed ADL-N profile. Context
 // operations remain inside the GPU LRI packet, never CPU MMIO programming.
 for(unsigned i=0;i<sRenderProfile.gtCount;i++)if(!NativeRenderApply(data,sRenderProfile.gt[i]))return false;
 for(unsigned i=0;i<sRenderProfile.engineCount;i++)if(!NativeRenderApply(data,sRenderProfile.engine[i]))return false;
 if(!NativeCachePolicy(data))return false;
 }
 sRenderRetired=false;
 if(draw && !(sRenderLargeDraw
  ? NativeGpu::RcsDrawImage64::Build(sRenderData,sRenderCombinedDraw?0:r.variant)
  : sRenderPublicBatchDraw
  ? NativeGpu::RcsUserVertices::Build(sRenderData,sRenderUserVertices)
  : sRenderArenaDraw && sRenderIndexedMesh
  ? NativeGpu::RcsDrawImage::Build(sRenderData,0)
  : sRenderArenaDraw
  ? NativeGpu::RcsResourceVertices::BuildAt(sRenderData,sRenderUserVertices,sRenderArenaGpu)
  : sRenderResourceDraw
  ? NativeGpu::RcsResourceVertices::Build(sRenderData,(uint32_t*)sRenderResourcePage.cpu,sRenderUserVertices)
  : sRenderUserDraw
   ? NativeGpu::RcsUserVertices::Build(sRenderData,sRenderUserVertices)
   : NativeGpu::RcsDrawImage::Build(sRenderData,r.variant)))return false;
 if(draw && (sRenderLargeDraw || sRenderIndexedMesh) && sRenderCombinedDraw
  && !NativeGpu::RcsResourceVertices::PatchAddressOnly(sRenderData,sRenderArenaGpu))
  return false;
 if(draw && sRenderPublicBatchDraw){
  memset(sRenderData[NativeGpu::RcsDrawImage::Batch],0,4096);
  memset(sRenderData[NativeGpu::RcsDrawImage::Vertices],0,4096);
 }
 if(draw && sRenderPublicShaderDraw){
  bool patched=sRenderCombinedDraw
   ?NativeGpu::RcsPublicShader::PatchBatchWithVertex(
    sRenderData[NativeGpu::RcsDrawImage::Batch],1024,
    sRenderPublicShaderGpu,sRenderArenaGpu,sRenderLargeDraw,
    sRenderFragmentProgram)
   :NativeGpu::RcsPublicShader::PatchBatch(
    sRenderData[NativeGpu::RcsDrawImage::Batch],1024,sRenderPublicShaderGpu);
  if(!patched)return false;
  // A passing draw must fetch the FS from the owner-addressed public BO.
  memset(sRenderData[NativeGpu::RcsDrawImage::Fs],0,4096);
 }
 if(draw && sRenderTwoDraw && !sRenderIndexedPair
  && !NativeGpu::RcsTwoDrawBatch::AppendContiguous(
   sRenderData[NativeGpu::RcsDrawImage::Batch],1024))return false;
 if(draw && sRenderIndexedDraw){
  if(!NativeGpu::RcsIndexedBatch::Patch(
   sRenderData[NativeGpu::RcsDrawImage::Batch],1024,
   sRenderIndexCount,sRenderVertexBytes))return false;
  memcpy((uint8_t*)sRenderData[NativeGpu::RcsDrawImage::Vertices]+256,
   sRenderIndices,sRenderIndexCount*2);
 }
 if(draw && sRenderPublicVertexDraw)
  memcpy(sRenderData[NativeGpu::RcsDrawImage::Vs],sRenderPublicVertexCode,4096);
 if(draw && sRenderDirectTarget){
  if(!NativeGpu::RcsResourceTarget::Patch(sRenderData,sRenderTargetGpu,
   sRenderLargeDraw?64:32))return false;
  if(sRenderPreserveTarget){
   if(!NativeVisible(data))return false;
   for(unsigned p=0;p<(sRenderLargeDraw?4u:1u);p++)
    memcpy(sRenderPreviousTarget+p*1024,sRenderTargetPages[p],4096);
  }else for(unsigned p=0;p<(sRenderLargeDraw?4u:1u);p++)
    for(unsigned i=0;i<1024;i++)
     sRenderTargetPages[p][i]=NativeGpu::RcsDrawImage::Poison;
 }
 if(draw && sRenderUniformDraw
  && !NativeGpu::RcsUniformSurface::Patch(sRenderData,
   sRenderPublicShaderGpu,sRenderUniformGpu))return false;
 r.stage=8;
 // Publish() performs the visibility flush immediately before GPU submission.
 // Rearm() only inspects state, so a draw needs no earlier full-cache flush.
 if(!NativeRenderBuild(r.nonce,r.sequence,r.ring_tail,draw)
  || (!draw && !NativeVisible(data)))return false;
 uint32_t tag=1+(r.sequence-1)%0x7fe;uint64_t descriptor=0;
 if(!intel_n100_rcs_descriptor(r.identity,uint8_t(r.revision),sRenderBuffers[3].ggtt_offset,tag,&descriptor))return false;
 NativeGpu::RcsEngine::IO engineIO={data,HandshakeTime,NativeRenderSafe,NativeRenderRawRead,NativeRenderRawWrite,NativeVisible};
 if(draw){if(!sRenderEngine.Rearm(engineIO,deadline))return false;}
 else if(!sRenderEngine.Initialize(engineIO,r.identity,r.revision,sRenderBuffers[1].ggtt_offset,deadline,0))return false;
 sNativeRenderInitialized=true;r.stage=9;
 // The PPGTT is resident after the first RCS publish. Never mutate a leaf
 // without a separately validated GPU TLB invalidation and retirement path.
 if(!sRenderTree.BindingsSealed() && !sRenderTree.SealBindings())return false;
 sRenderPublished=true;
 if(!sRenderEngine.Publish(engineIO,descriptor))return false;
#ifdef NATIVE_RENDER_PROFILE_TIMING
 sRenderTiming.published=system_time();
#endif
 r.stage=10;
 sRenderCsb.~IntelN100CsbReader();new (&sRenderCsb) IntelN100CsbReader;
 IntelN100CsbReader::Io csb={data,HandshakeTime,NativeRenderSafe,NativeRenderStatus32,
  NativeRenderStatus64,NativeAcquire,HandshakePause};
 uint64_t now=HandshakeTime(NULL),budget=deadline>now+10000?deadline-now-10000:0;
 if(budget>20000)budget=20000;
 if(!budget || !sRenderCsb.Run(csb,sMappingLease,tag,budget,2048,sRenderCursor,0)){
  auto error=sRenderCsb.LastReport().error;
  r.status=!budget || error==IntelN100CsbReader::Deadline
   || error==IntelN100CsbReader::PollLimit?B_TIMED_OUT:B_IO_ERROR;
  return false;
 }
#ifdef NATIVE_RENDER_PROFILE_TIMING
 sRenderTiming.csbDone=system_time();
#endif
 r.activation=sRenderCsb.LastReport().events.raw[0];r.completion=sRenderCsb.LastReport().events.raw[1];
 if(!NativeRenderSafe(data) || !NativeVisible(data))return false;
 const volatile uint32_t* result=(const volatile uint32_t*)sRenderBuffers[2].cpu_aperture;
 const volatile uint32_t* status=(const volatile uint32_t*)sRenderBuffers[1].cpu_aperture;
 r.observed_nonce=result[0];r.observed_complement=result[1];r.observed_sequence=status[0x40];
 uint32_t idle=0;
 if(r.observed_nonce!=r.nonce || r.observed_complement!=~r.nonce || r.observed_sequence!=r.sequence
  || !NativeRenderRawRead(data,0x209c,idle) || !(idle&0x200) || !NativeRenderSafe(data))return false;
 // A completed, idle version-12 diagnostic may return raw pixels even when
 // the new CPU image reference disagrees. The ioctl still reports failure.
 sRenderGradientCaptured=r.version==12;
 // Only switch-away + ordered breadcrumb + visibility + idle permits saving
 // the complete ppHWSP/register-image default. Keep both WA pages in place.
 if(draw){
  uint32_t* directPages[NativeGpu::RcsDrawImage64::PageCount];
  uint32_t* const* resultPages=sRenderData;
  if(sRenderDirectTarget){
   memcpy(directPages,sRenderData,sizeof(directPages));
   for(unsigned p=0;p<(sRenderLargeDraw?4u:1u);p++)
    directPages[NativeGpu::RcsDrawImage::Target+p]=sRenderTargetPages[p];
   resultPages=directPages;
  }
  bool matched=false;
  if(sRenderIndexedMesh){
   // This request reports completion and a full readback hash, but makes no
   // fixed-geometry CPU pixel claim for application-supplied triangles.
   uint32_t hash=2166136261u;
   for(unsigned p=0;p<(sRenderLargeDraw?4u:1u);p++)
    for(unsigned i=0;i<1024;i++)
     hash=(hash^resultPages[NativeGpu::RcsDrawImage::Target+p][i])*16777619u;
   r.target_hash=hash;matched=true;
  }else if(sRenderTwoDraw){
   NativeGpu::RcsUserVertices::Check check={};
   matched=sRenderLargeDraw
    ?NativeGpu::RcsUserVertices::InspectPair64(resultPages,sRenderUserVertices,
      sRenderSecondVertices,check,sRenderExpectedPixel,
      sRenderPreserveTarget?sRenderPreviousTarget:nullptr)
    :NativeGpu::RcsUserVertices::InspectPair(
      resultPages[NativeGpu::RcsDrawImage::Target],sRenderUserVertices,
      sRenderSecondVertices,check,sRenderExpectedPixel,
      sRenderPreserveTarget?sRenderPreviousTarget:nullptr);
   r.checked_pixels=check.checked;r.matched_pixels=check.matched;
   r.target_hash=check.hash;r.interior_xy=check.interior;
   r.exterior_xy=check.exterior;
  }else if(sRenderLargeDraw && sRenderCombinedDraw){
   NativeGpu::RcsUserVertices::Check check={};
   matched=NativeGpu::RcsUserVertices::Inspect64(
    resultPages,sRenderPublicVertexDraw?sRenderExpectedVertices:
     sRenderUserVertices,check,sRenderExpectedPixel,sRenderGradientDenominator,
    sRenderGradientGreen,sRenderPreserveTarget?sRenderPreviousTarget:nullptr);
   r.checked_pixels=check.checked;r.matched_pixels=check.matched;r.target_hash=check.hash;
   r.interior_xy=check.interior;r.exterior_xy=check.exterior;
  }else if(sRenderLargeDraw){
   NativeGpu::RcsRequestReuse::Check check={};
   matched=NativeGpu::RcsRequestReuse::Inspect64(
    resultPages,r.variant,check,sRenderExpectedPixel);
   r.checked_pixels=check.checked;r.matched_pixels=check.matched;r.target_hash=check.hash;
   r.interior_xy=check.interior;r.exterior_xy=check.exterior;
  }else if(sRenderUserDraw){
   NativeGpu::RcsUserVertices::Check check={};
   matched=NativeGpu::RcsUserVertices::Inspect(
    resultPages[NativeGpu::RcsDrawImage::Target],
    sRenderPublicVertexDraw?sRenderExpectedVertices:sRenderUserVertices,check,
    sRenderExpectedPixel,sRenderPreserveTarget?sRenderPreviousTarget:nullptr);
   r.checked_pixels=check.checked;r.matched_pixels=check.matched;r.target_hash=check.hash;
   r.interior_xy=check.interior;r.exterior_xy=check.exterior;
  }else{
   NativeGpu::RcsRequestReuse::Check check={};
   matched=NativeGpu::RcsRequestReuse::Inspect(
    resultPages[NativeGpu::RcsDrawImage::Target],r.variant,check,
    sRenderExpectedPixel);
   r.checked_pixels=check.checked;r.matched_pixels=check.matched;r.target_hash=check.hash;
   r.interior_xy=check.interior;r.exterior_xy=check.exterior;
  }
  if(!matched || !NativeRenderSafe(data))return false;
 }else memcpy(sRenderSavedDefault,(void*)sRenderBuffers[3].cpu_aperture,sizeof(sRenderSavedDefault));
 sRenderCursor=sRenderCsb.LastReport().tail;
#ifdef NATIVE_RENDER_PROFILE_TIMING
 sRenderTiming.verified=system_time();
#endif
 r.stage=11;return true;
}
// Initial admission diagnostic only, never used by hot PCI/power accessors.
// Fixed 512-byte records; no pointers or CPU addresses enter the log.
struct NativeRenderPciTrace {
 struct Entry {uint32_t access,value;};
 Entry entries[64];
 void* device;
 uint32_t count,total;
};
static_assert(sizeof(NativeRenderPciTrace)<=544,"bounded initial PCI trace");
static bool NativeRenderTracePciRead(void* data,uint16_t offset,uint8_t size,uint32_t& value)
{
 auto& trace=*(NativeRenderPciTrace*)data;
 value=0;bool read=ReadConfig(trace.device,offset,size,value);
 ++trace.total;
 if(trace.count<64)trace.entries[trace.count++]={uint32_t(offset)|(uint32_t(size)<<16)|(uint32_t(read)<<24),value};
 // Normalize only after retaining the raw diagnostic. The full Capture still
 // performs the same two walks and all other exact comparisons/failure checks.
 if(read)value=NativeGpu::RcsPciAdmission::Normalize(offset,size,value);
 return read;
}
static IntelN100PciPower::Result NativeRenderCapturePci(IntelN100PciSnapshot& snapshot)
{
 NativeRenderPciTrace trace={};trace.device=&sDevice;
 auto result=IntelN100PciPower::Capture(NativeRenderTracePciRead,&trace,sDevice.base,sDevice.size,snapshot);
 if(result!=IntelN100PciPower::Success){
  dprintf("native RCS PCI Capture failed result=%u records=%u total=%u truncated=%u\n",
   uint32_t(result),trace.count,trace.total,uint32_t(trace.total>trace.count));
  for(unsigned i=0;i<trace.count;i++){
   const auto& e=trace.entries[i];
   dprintf("native RCS PCI read[%u] offset=%04x size=%u value=%08x success=%u\n",
    i,e.access&0xffff,(e.access>>16)&255,e.value,(e.access>>24)&1);
  }
 }
 return result;
}
static status_t RunNativeRenderOwned(native_gpu_render_command* command,
 uint64_t resourceSession,uint8_t* largeExtra=nullptr)
{
 if(!command)return B_BAD_VALUE;
 uint32_t version=command->version,mode=command->mode,variant=command->variant;
 bool pairDraw=version==20 || version==21;
 bool indexedDraw=(version==22 || version==23 || version==24
  || version==25)
  && mode==NATIVE_GPU_RENDER_DRAW;
 bool indexedPair=version==24 && indexedDraw;
 bool indexedMesh=version==25 && indexedDraw;
 uint32_t uniformHandle=0;
 if(version>=16 && version<=19)
  memcpy(&uniformHandle,command->output+12,sizeof(uniformHandle));
 bool uniformDraw=(version==16 || version==17
  || ((version==18 || version==19) && uniformHandle!=0))
  && mode==NATIVE_GPU_RENDER_DRAW;
 bool largeDraw=(version==9 || version==10 || version==11 || version==12
  || version==13 || version==14 || version==15 || version==16
  || version==17 || version==18 || version==19 || pairDraw || indexedDraw)
  && mode==NATIVE_GPU_RENDER_DRAW && largeExtra;
 bool resourceDraw=version==3 && mode==NATIVE_GPU_RENDER_DRAW;
 bool arenaDraw=version==4 && mode==NATIVE_GPU_RENDER_DRAW;
 bool publicBatchDraw=version==5 && mode==NATIVE_GPU_RENDER_DRAW;
 bool publicShaderDraw=version==6 && mode==NATIVE_GPU_RENDER_DRAW;
 uint32_t directVertexShaderHandle=0;
 if(version>=15 && version<=25)memcpy(&directVertexShaderHandle,command->output+4,
  sizeof(directVertexShaderHandle));
 bool publicVertexDraw=(version==8 || version==11 || version==14
  || ((version==15 || version==18 || version==19) && !uniformDraw
   && directVertexShaderHandle!=0))
  && mode==NATIVE_GPU_RENDER_DRAW;
 bool combinedDraw=(version==7 || version==10 || version==12
  || version==13 || version==15 || version==18 || version==19
  || pairDraw || indexedDraw
  || uniformDraw || publicVertexDraw)
  && mode==NATIVE_GPU_RENDER_DRAW;
 bool userDraw=(version==2 || resourceDraw) && mode==NATIVE_GPU_RENDER_DRAW;
 uint32_t userVertices[12]={};
 uint32_t secondVertices[12]={};
 uint16_t indices[6]={};
 uint32_t expectedVertices[12]={};
 if(userDraw)memcpy(userVertices,command->output,sizeof(userVertices));
 uint32_t vertexShaderHandle=0;
 if(publicVertexDraw)memcpy(&vertexShaderHandle,
  command->output+((version==14 || version==15 || version==18 || version==19
   || uniformDraw)?4:0),sizeof(vertexShaderHandle));
 uint32_t vertexOffset=0;
 if(version==13 || version==14 || version==15 || version==18
  || version==19 || pairDraw || indexedDraw || uniformDraw)
  memcpy(&vertexOffset,command->output,sizeof(vertexOffset));
 uint32_t targetHandle=0;
 if(version==15 || version==18 || version==19 || pairDraw || indexedDraw
  || uniformDraw)
  memcpy(&targetHandle,command->output+8,sizeof(targetHandle));
 uint32_t uniformOffset=0;
 if(version==17 || version==18 || version==19)
  memcpy(&uniformOffset,command->output+16,sizeof(uniformOffset));
 uint32_t targetOffset=0;
 if(version==18 || version==19)
  memcpy(&targetOffset,command->output+20,sizeof(targetOffset));
 uint32_t indexHandle=0,indexOffset=0;
 if(indexedDraw){
  memcpy(&indexHandle,command->output+24,sizeof(indexHandle));
  memcpy(&indexOffset,command->output+28,sizeof(indexOffset));
 }
 uint32_t meshVertexCount=0,meshIndexCount=0,meshFlags=0;
 if(indexedMesh){
  memcpy(&meshVertexCount,command->output+12,sizeof(meshVertexCount));
  memcpy(&meshIndexCount,command->output+16,sizeof(meshIndexCount));
  memcpy(&meshFlags,command->output+20,sizeof(meshFlags));
 }
 const uint32_t selectedVertexBytes=indexedMesh?meshVertexCount*16:
  pairDraw?96u:indexedDraw?64u:48u;
 const uint32_t selectedIndexBytes=indexedMesh?meshIndexCount*2:
  indexedPair?12u:6u;
 bool valid=(command->reserved==0
  && ((version==1 && ((mode==NATIVE_GPU_RENDER_DRAW && variant<4)
    || ((mode==NATIVE_GPU_RENDER_PROBE || mode==NATIVE_GPU_RENDER_BOOTSTRAP) && variant==0)))
   || (userDraw && variant==0 && NativeGpu::RcsUserVertices::Validate(userVertices))
   || (largeDraw && version==9 && variant<4)))
  || (arenaDraw && variant==0 && command->reserved!=0 && resourceSession!=0)
  || (publicBatchDraw && variant!=0 && command->reserved!=0 && resourceSession!=0)
  || (publicShaderDraw && variant<4 && command->reserved!=0 && resourceSession!=0)
  || (combinedDraw && (!indexedMesh || (meshVertexCount>=3
    && meshVertexCount<=256 && meshIndexCount>=3
    && meshIndexCount<=255 && meshIndexCount%3==0 && meshFlags<=1))
   && (!largeDraw || version==10 || version==11
    || version==12 || version==13 || version==14 || version==15
    || version==16 || version==17 || version==18 || version==19
    || pairDraw || indexedDraw)
   && variant!=0 && command->reserved!=0
   && variant!=command->reserved && resourceSession!=0
   && ((version!=13 && version!=14 && version!=15 && version!=18
    && version!=19 && !pairDraw && !indexedDraw && !uniformDraw)
    || ((version==15 || version==18 || version==19 || pairDraw
     || indexedDraw
     || uniformDraw
     || vertexOffset>=16)
    && vertexOffset<=NativeGpu::RcsResourceRange::Pages*4096
     -selectedVertexBytes
    && !(vertexOffset&15)
    && (vertexOffset&4095)<=4096-selectedVertexBytes))
   && ((version!=15 && version!=18 && version!=19
    && !pairDraw && !indexedDraw && !uniformDraw)
    || (targetHandle!=0 && targetHandle!=variant
    && targetHandle!=command->reserved
    && targetHandle!=vertexShaderHandle))
   && ((version!=18 && version!=19)
    || ((version!=18 || targetOffset!=0) && !(targetOffset&4095)
    && targetOffset<=NativeGpu::RcsResourceRange::Pages*4096
     -(largeDraw?16384u:4096u)
     && (uniformDraw || uniformOffset==0)))
   && (!pairDraw || directVertexShaderHandle==0)
   && (!indexedDraw || (directVertexShaderHandle==0
    && indexHandle!=0 && indexHandle!=variant
    && indexHandle!=command->reserved && indexHandle!=targetHandle
    && !(indexOffset&1)
    && indexOffset<=NativeGpu::RcsResourceRange::Pages*4096
     -selectedIndexBytes
    && (indexOffset&4095)<=4096-selectedIndexBytes))
   && (!uniformDraw || (directVertexShaderHandle==0 && uniformHandle!=0
    && uniformHandle!=variant && uniformHandle!=command->reserved
    && uniformHandle!=targetHandle
    && !(uniformOffset&15) && uniformOffset<=NativeGpu::RcsResourceRange::Pages*4096-16
    && (uniformOffset&4095)<=4096-16))
   && (!publicVertexDraw || (vertexShaderHandle!=0
    && vertexShaderHandle!=variant && vertexShaderHandle!=command->reserved)));
 // The bridge stages on the heap; the owner never retains that pointer.
 command->report={};memset(command->output,0,sizeof(command->output));
 auto& output=command->report;output.version=version;output.mode=mode;output.variant=variant;
 output.status=B_BAD_VALUE;
 if(!valid)return output.status;

 mutex_lock(&sLock);
 uint64_t arenaGpu=0,publicBatchGpu=0,publicShaderGpu=0;
 const uint32_t* vertexWords=nullptr;
 uint32_t renderedPixel=NativeGpu::RcsDrawImage::RenderedPixel;
 auto fragmentProgram=NativeGpu::RcsPublicShader::ConstantColor;
 unsigned gradientDenominator=0;
 if(publicShaderDraw || combinedDraw){
  if(!NativeResourceAllowedLocked()
   || !sRenderArena.Resolve(resourceSession,command->reserved,0,4096,publicShaderGpu)){
   mutex_unlock(&sLock);return output.status;
  }
  unsigned shaderPage=unsigned((publicShaderGpu-kRenderArenaGpu)/4096);
  const uint32_t* shaderWords=(const uint32_t*)sRenderArenaPages[shaderPage].cpu;
  fragmentProgram=uniformDraw?NativeGpu::RcsPublicShader::ConstantColor:
   NativeGpu::RcsPublicShader::IdentifyFragmentPage(shaderWords,1024);
  bool gradient=fragmentProgram==NativeGpu::RcsPublicShader::FragCoordRed
   || fragmentProgram==NativeGpu::RcsPublicShader::FragCoordGreen;
  if(gradient)
   gradientDenominator=NativeGpu::RcsPublicShader::GradientDenominator(shaderWords[29]);
  bool accepted=uniformDraw
   ?NativeGpu::RcsUniformShader::ValidatePage(shaderWords,1024):pairDraw
   ?fragmentProgram==NativeGpu::RcsPublicShader::ConstantColor:version==12
   ?gradient:
   (version==13 || version==15 || version==18 || version==19
    || pairDraw)
    ?fragmentProgram==NativeGpu::RcsPublicShader::ConstantColor
    || (largeDraw && gradient):
   fragmentProgram==NativeGpu::RcsPublicShader::ConstantColor;
  if(!accepted || (!uniformDraw
   && fragmentProgram==NativeGpu::RcsPublicShader::ConstantColor
   && !NativeGpu::RcsPublicShader::ExpectedPixel(
    shaderWords,1024,renderedPixel))){
   mutex_unlock(&sLock);return output.status;
  }
 }
 if(arenaDraw || publicBatchDraw || combinedDraw){
  if(!NativeResourceAllowedLocked()){
   output.status=B_DEV_NOT_READY;mutex_unlock(&sLock);return output.status;
  }
  uint32_t vertexHandle=publicBatchDraw || combinedDraw?variant:command->reserved;
  if(!sRenderArena.Resolve(resourceSession,vertexHandle,vertexOffset,
    selectedVertexBytes,arenaGpu)){
   mutex_unlock(&sLock);return output.status;
  }
  unsigned page=unsigned((arenaGpu-kRenderArenaGpu)/4096);
  const uint8_t* vertexSource=(const uint8_t*)sRenderArenaPages[page].cpu
   +(arenaGpu&4095);
  if(indexedDraw){
   uint64_t indexGpu=0;
   if(!sRenderArena.Resolve(resourceSession,indexHandle,indexOffset,
     selectedIndexBytes,indexGpu) || (indexGpu&1)){
    mutex_unlock(&sLock);return output.status;
   }
   unsigned indexPage=unsigned((indexGpu-kRenderArenaGpu)/4096);
   uint16_t* selected=indexedMesh?sRenderIndices:indices;
   memcpy(selected,(const uint8_t*)sRenderArenaPages[indexPage].cpu
    +(indexGpu&4095),selectedIndexBytes);
   for(unsigned i=0;i<(indexedMesh?meshIndexCount:
     indexedPair?6u:3u);i++){
    if(selected[i]>=(indexedMesh?meshVertexCount:4u)){
     mutex_unlock(&sLock);return output.status;
    }
    if(!indexedMesh)memcpy((i<3?userVertices:secondVertices)+(i%3)*4,
     vertexSource+selected[i]*16,16);
   }
   if(indexedMesh)for(unsigned i=0;i<meshVertexCount;i++){
    const uint32_t* position=(const uint32_t*)(vertexSource+i*16);
    // Keep input coordinates finite and bounded before the GPU reads them.
    bool validPosition=true;
    for(unsigned component=0;component<3;component++)
     validPosition &=(position[component]&0x7fffffff)<=0x41800000;
    validPosition &=position[3]>=0x3d800000
     && position[3]<=0x41800000;
    if(!validPosition){mutex_unlock(&sLock);return output.status;}
   }
  }else memcpy(userVertices,vertexSource,sizeof(userVertices));
  if(!indexedMesh && !NativeGpu::RcsUserVertices::Validate(userVertices)){
   mutex_unlock(&sLock);return output.status;
  }
  if(pairDraw){
   memcpy(secondVertices,(const uint8_t*)sRenderArenaPages[page].cpu
    +(arenaGpu&4095)+sizeof(userVertices),sizeof(secondVertices));
  }
  if((pairDraw || indexedPair)
   && !NativeGpu::RcsUserVertices::Validate(secondVertices)){
   mutex_unlock(&sLock);return output.status;
  }
  if(publicBatchDraw){
   if(!sRenderArena.Resolve(resourceSession,command->reserved,0,4096,publicBatchGpu)
    || publicBatchGpu==arenaGpu){mutex_unlock(&sLock);return output.status;}
   unsigned batchPage=unsigned((publicBatchGpu-kRenderArenaGpu)/4096);
   if(!NativeGpu::RcsValidatedBatch::Validate(
    (const uint32_t*)sRenderArenaPages[batchPage].cpu,1024,uint32_t(arenaGpu))){
    mutex_unlock(&sLock);return output.status;
   }
  }
 }
 if(publicVertexDraw){
  uint64_t vertexShaderGpu=0;
  if(!NativeResourceAllowedLocked()
   || !sRenderArena.Resolve(resourceSession,vertexShaderHandle,0,4096,vertexShaderGpu)){
   mutex_unlock(&sLock);return output.status;
  }
  unsigned page=unsigned((vertexShaderGpu-kRenderArenaGpu)/4096);
  vertexWords=(const uint32_t*)sRenderArenaPages[page].cpu;
  auto program=NativeGpu::RcsPublicShader::IdentifyVertexPage(vertexWords,1024);
  if(!NativeGpu::RcsPublicShader::ExpectedVertices(
   program,userVertices,expectedVertices)){
   mutex_unlock(&sLock);return output.status;
  }
 }
 uint64_t targetGpu=0;
 if(version==15 || version==18 || version==19 || pairDraw || indexedDraw
  || uniformDraw){
  unsigned bytes=largeDraw?16384:4096;
  if(!sRenderArena.Resolve(resourceSession,targetHandle,targetOffset,bytes,targetGpu)
   || (targetGpu&4095)){
   mutex_unlock(&sLock);return output.status;
  }
 }
 uint64_t uniformGpu=0;
 if(uniformDraw){
  if(!sRenderArena.Resolve(resourceSession,uniformHandle,uniformOffset,16,uniformGpu)
   || (uniformGpu&15)){
   mutex_unlock(&sLock);return output.status;
  }
  const uint32_t* rgba=(const uint32_t*)((const uint8_t*)sRenderArenaPages[
   unsigned((uniformGpu-kRenderArenaGpu)/4096)].cpu+(uniformGpu&4095));
  if(!NativeGpu::RcsUniformShader::ExpectedPixel(rgba,renderedPixel)){
   mutex_unlock(&sLock);return output.status;
  }
 }
 NativeRenderSnapshotLocked(output);
 if(mode!=NATIVE_GPU_RENDER_PROBE){
  output.refusal=!NativeRenderOwnerLocked()?NATIVE_GPU_RENDER_NO_HANDOFF:
   sNativeSequence==UINT32_MAX || sRenderOperationSerial==UINT64_MAX?NATIVE_GPU_RENDER_SEQUENCE_EXHAUSTED:
   mode==NATIVE_GPU_RENDER_DRAW && (!sRenderDefaultValid || !sNativeRenderInitialized)?NATIVE_GPU_RENDER_NEEDS_BOOTSTRAP:
   resourceDraw && !sRenderResourceReady?NATIVE_GPU_RENDER_RESOURCE_UNAVAILABLE:
   mode==NATIVE_GPU_RENDER_BOOTSTRAP && sNativeRenderInitialized?NATIVE_GPU_RENDER_ALREADY_BOOTSTRAPPED:0;
  if(output.refusal){output.status=B_NOT_ALLOWED;mutex_unlock(&sLock);return output.status;}
 }
 if(gKernelShutdown)sNativeLifecycle.Freeze(NativeGpu::RequestLifecycle::Shutdown);
 auto admission=sNativeLifecycle.Begin();
 if(admission!=NativeGpu::RequestLifecycle::Accepted){
  mutex_unlock(&sLock);
  output.status=admission==NativeGpu::RequestLifecycle::Busy?B_BUSY:B_DEV_NOT_READY;
  return output.status;
 }
 if(publicVertexDraw)memcpy(sRenderPublicVertexCode,vertexWords,sizeof(sRenderPublicVertexCode));
 sRenderUserDraw=userDraw || arenaDraw || publicBatchDraw || combinedDraw;
 sRenderLargeDraw=largeDraw;
 sRenderResourceDraw=resourceDraw;
 sRenderArenaDraw=arenaDraw || combinedDraw;sRenderArenaGpu=arenaGpu;
 sRenderPublicBatchDraw=publicBatchDraw;sRenderPublicBatchGpu=publicBatchGpu;
 sRenderPublicShaderDraw=publicShaderDraw || combinedDraw;
 sRenderPublicVertexDraw=publicVertexDraw;
 sRenderDirectTarget=version==15 || version==18 || version==19
  || pairDraw || indexedDraw || uniformDraw;
 sRenderPreserveTarget=version==19 || version==21 || version==23
  || (indexedMesh && (meshFlags&1));
 sRenderTwoDraw=pairDraw || indexedPair;
 sRenderIndexedDraw=indexedDraw;
 sRenderIndexedPair=indexedPair;
 sRenderIndexedMesh=indexedMesh;
 sRenderIndexCount=indexedMesh?meshIndexCount:indexedPair?6:3;
 sRenderVertexBytes=selectedVertexBytes;
 sRenderTargetGpu=targetGpu;
 sRenderUniformDraw=uniformDraw;sRenderUniformGpu=uniformGpu;
 if(sRenderDirectTarget){
  unsigned first=unsigned((targetGpu-kRenderArenaGpu)/4096);
  for(unsigned p=0;p<(largeDraw?4u:1u);p++)
   sRenderTargetPages[p]=(uint32_t*)sRenderArenaPages[first+p].cpu;
 }
 sRenderPublicShaderGpu=publicShaderGpu;sRenderCombinedDraw=combinedDraw;
 sRenderExpectedPixel=renderedPixel;
 sRenderFragmentProgram=fragmentProgram;
 sRenderGradientDenominator=gradientDenominator;
 sRenderGradientGreen=fragmentProgram==NativeGpu::RcsPublicShader::FragCoordGreen;
 sRenderGradientCaptured=false;
 if(userDraw || arenaDraw || publicBatchDraw || combinedDraw)
  memcpy(sRenderUserVertices,userVertices,sizeof(userVertices));
 if(pairDraw || indexedPair)
  memcpy(sRenderSecondVertices,secondVertices,sizeof(secondVertices));
 if(indexedDraw && !indexedMesh)
  memcpy(sRenderIndices,indices,selectedIndexBytes);
 if(publicVertexDraw)
  memcpy(sRenderExpectedVertices,expectedVertices,sizeof(expectedVertices));
 sRenderReport=output;auto& r=sRenderReport;
#ifdef NATIVE_RENDER_PROFILE_TIMING
 sRenderTiming={};sRenderTiming.accepted=system_time();
 sNativeRenderVisibleUs=0;sNativeRenderVisibleCalls=0;
#endif
 if(mode!=NATIVE_GPU_RENDER_PROBE){
  ++sNativeSequence;if(sNativeSequence==NativeGpu::kPoison)++sNativeSequence;
  r.sequence=sNativeSequence;
  if(sRenderOwnership.Begin(sRenderDisplayLease,sRenderHandoff,++sRenderOperationSerial)!=NativeGpu::RcsOwnership::Accepted){
   sNativeLifecycle.Finish(false);mutex_unlock(&sLock);output.status=B_DEV_NOT_READY;return output.status;
  }
  sRenderActiveOperation=sRenderOperationSerial;
  r.nonce=(uint32_t(system_time())^r.sequence)|1;
  if(r.nonce==NativeGpu::kPoison || ~r.nonce==NativeGpu::kPoison)r.nonce^=0x915;
 }
 r.owner_granted=NativeRenderOwnerLocked();
 r.refusal=r.owner_granted?0:NATIVE_GPU_RENDER_NO_HANDOFF;
 mutex_unlock(&sLock);
 bool retired=false,releaseObservation=false;
 struct Scope {
  bool& retired;bool& releaseObservation;uint32_t mode;
  ~Scope(){mutex_lock(&sLock);sNativeLastStatus=sRenderReport.status;
   sNativeRenderActive=false;
   if(mode!=NATIVE_GPU_RENDER_PROBE)
    sRenderOwnership.Finish(sRenderDisplayLease,sRenderHandoff,sRenderActiveOperation,retired);
   if(mode==NATIVE_GPU_RENDER_PROBE || releaseObservation)sNativeLifecycle.FinishObservation(retired);
   else sNativeLifecycle.Finish(retired);
   mutex_unlock(&sLock);}
 } scope{retired,releaseObservation,mode};
 r.stage=1;r.status=B_IO_ERROR;
 r.mapping=NativeEnsureMapping();
 if(r.mapping!=B_OK){output=r;return r.status;}
 r.dma=B_OK;r.retained=1;
 if(mode!=NATIVE_GPU_RENDER_PROBE && !NativeRenderAllocate()){output=r;return r.status;}
 IntelN100PciSnapshot original={};
 r.pci_error=NativeRenderCapturePci(original);
 if(r.pci_error!=IntelN100PciPower::Success){output=r;return r.status;}
 r.identity=original.identity;r.revision=uint8_t(original.classRevision);
 mutex_lock(&sLock);
 if(sObservationBlocked || sObservationPhase!=ObservationIdle
  || (sGroupAttempted && !sNativeGroupReusable) || !sState.DiagnosticAllowed()){
  // No forcewake or GPU publication occurred. Preserve global admission.
  retired=true;releaseObservation=true;r.stage=0;r.status=B_BUSY;NativeRenderSnapshotLocked(r);mutex_unlock(&sLock);output=r;return r.status;
 }
 if(mode==NATIVE_GPU_RENDER_DRAW)
  sRenderHideCompletion=NativeGpu::RcsCompletionObservation::ConsumeTimeout(sNativeTestTimeout,mode,true);
 if(sNativeGroupReusable){sGroup.~IntelN100ForcewakeGroup();new (&sGroup) IntelN100ForcewakeGroup;}
 sNativeGroupReusable=false;sGroupAttempted=true;
 sObservationPhase=ObservationHandshaking;sObservationLease=sMappingLease;
 sObservationSnapshot=original;sNativeExecutionActive=true;
 mutex_unlock(&sLock);
 uint32 mediaFuse=0;
 HandshakeContext h={sDevice,original,sMappingLease,sResources.registers,NULL,&mediaFuse,false,0,0};
 IntelN100Forcewake::IO io={&h,HandshakeTime,HandshakeRead,HandshakeWrite,
  HandshakePciStable,HandshakeLive,HandshakePause};
 r.stage=2;
#ifdef NATIVE_RENDER_PROFILE_TIMING
 sRenderTiming.forcewakeStart=system_time();
#endif
 r.bootstrap=sForcewake.RunWithInspection(io,InspectInventory)==IntelN100Forcewake::Ok?B_OK:B_ERROR;
#ifdef NATIVE_RENDER_PROFILE_TIMING
 sRenderTiming.forcewakeDone=system_time();
#endif
 GroupContext context={&h,{},0,0,{}};
 if(r.bootstrap==B_OK && NativeOwnerPower(context)
  && intel_n100_reset_domains(original.identity,uint8(original.classRevision),mediaFuse,context.plan)){
  IntelN100ForcewakeGroup::IO group={&context,HandshakeTime,NativeRenderGroupSafe,GroupRead,GroupWrite,HandshakePause};
  r.stage=3;
  r.group=sGroup.Run(group,original.identity,uint8(original.classRevision),mediaFuse,mode==NATIVE_GPU_RENDER_PROBE?NativeRenderProbe:NativeRenderRequest)
   ==IntelN100ForcewakeGroup::Ok?B_OK:B_ERROR;
#ifdef NATIVE_RENDER_PROFILE_TIMING
  sRenderTiming.groupDone=system_time();
#endif
  retired=r.group==B_OK && r.stage==(mode==NATIVE_GPU_RENDER_PROBE?5U:11U)
   && NativeOwnerPower(context) && NativeDmaStillDirect();
 }
 r.bootstrap_stage=sForcewake.LastReport().stage;r.bootstrap_error=sForcewake.LastReport().error;
 r.group_stage=sGroup.LastReport().stage;r.group_error=sGroup.LastReport().error;
 r.safety_reason=context.safety.reason;
 r.engine_stage=sRenderEngine.LastReport().stage;r.engine_error=sRenderEngine.LastReport().error;
 r.csb_error=sRenderCsb.LastReport().error;
 mutex_lock(&sLock);
 if(mode!=NATIVE_GPU_RENDER_PROBE && retired)retired=NativeRenderOwnerLocked(true);
 sNativeGroupReusable=retired;
 if(mode!=NATIVE_GPU_RENDER_PROBE){
  sRenderDefaultValid=retired;sRenderRetired=retired;
 }
 if(retired){sObservationPhase=ObservationIdle;sObservationLease=0;
  if(mode!=NATIVE_GPU_RENDER_PROBE)sRenderArena.RetireOrphans(true);
  r.stage=mode==NATIVE_GPU_RENDER_PROBE?6:12;r.status=r.known_profile?B_OK:B_NOT_SUPPORTED;}
 else{sObservationPhase=ObservationUnknown;sObservationBlocked=true;}
 NativeRenderSnapshotLocked(r);output=r;
 if((retired || (version==12 && sRenderGradientCaptured))
  && mode==NATIVE_GPU_RENDER_DRAW)
  memcpy(command->output,sRenderDirectTarget?sRenderTargetPages[0]:
   sRenderData[NativeGpu::RcsDrawImage::Target],sizeof(command->output));
 if((retired || (version==12 && sRenderGradientCaptured)) && largeDraw)
  for(unsigned p=1;p<NativeGpu::RcsDrawImage64::TargetPages;p++)
   memcpy(largeExtra+(p-1)*4096,sRenderDirectTarget?sRenderTargetPages[p]:
    sRenderData[NativeGpu::RcsDrawImage64::Target+p],4096);
#ifdef NATIVE_RENDER_PROFILE_TIMING
 sRenderTiming.finished=system_time();
 if((version==15 || version==18 || version==19 || pairDraw || uniformDraw)
  && mode==NATIVE_GPU_RENDER_DRAW && r.status==B_OK)
  dprintf("native_rcs_timing seq=%u accepted_us=%lld forcewake_us=%lld "
   "group_before_work_us=%lld work_to_publish_us=%lld csb_wait_us=%lld "
   "verify_us=%lld group_after_work_us=%lld finish_us=%lld total_us=%lld "
   "visible_us=%lld visible_calls=%u\n",
   r.sequence,(long long)(sRenderTiming.forcewakeStart-sRenderTiming.accepted),
   (long long)(sRenderTiming.forcewakeDone-sRenderTiming.forcewakeStart),
   (long long)(sRenderTiming.workStart-sRenderTiming.forcewakeDone),
   (long long)(sRenderTiming.published-sRenderTiming.workStart),
   (long long)(sRenderTiming.csbDone-sRenderTiming.published),
   (long long)(sRenderTiming.verified-sRenderTiming.csbDone),
   (long long)(sRenderTiming.groupDone-sRenderTiming.verified),
   (long long)(sRenderTiming.finished-sRenderTiming.groupDone),
   (long long)(sRenderTiming.finished-sRenderTiming.accepted),
   (long long)sNativeRenderVisibleUs,sNativeRenderVisibleCalls);
#endif
 mutex_unlock(&sLock);return r.status;
}
static status_t RunNativeRender(native_gpu_render_command* command)
{return RunNativeRenderOwned(command,0);}
static status_t RunNativeRenderLarge(uint64_t resourceSession,
 native_gpu_render_large_command* command)
{
 if(!command)return B_BAD_VALUE;
 if(command->base.version!=9 && command->base.version!=10
  && command->base.version!=11 && command->base.version!=12
  && command->base.version!=13 && command->base.version!=14
  && command->base.version!=15 && command->base.version!=16
  && command->base.version!=17 && command->base.version!=18
  && command->base.version!=19 && command->base.version!=20
  && command->base.version!=21 && command->base.version!=22
  && command->base.version!=23 && command->base.version!=24
  && command->base.version!=25)return B_BAD_VALUE;
 memset(command->extra,0,sizeof(command->extra));
 return RunNativeRenderOwned(&command->base,resourceSession,command->extra);
}
static status_t NativeResourceDraw(uint64_t session,native_gpu_render_command* command)
{
 if(!command || (command->version!=4 && command->version!=5 && command->version!=6
  && command->version!=7 && command->version!=8
  && command->version!=13 && command->version!=14
  && command->version!=15 && command->version!=16 && command->version!=17
  && command->version!=18 && command->version!=19
  && command->version!=20 && command->version!=21
  && command->version!=22 && command->version!=23
  && command->version!=24 && command->version!=25)
  || command->mode!=NATIVE_GPU_RENDER_DRAW)return B_BAD_VALUE;
 return RunNativeRenderOwned(command,session);
}
// Administrative resume is serialized by Suspended + TransitionBusy. It owns
// a separate private ownership operation, never a synthetic GPU request seqno.
static bool NativeRenderResumeWork(void* data,uint64_t deadline)
{
 auto& context=*(GroupContext*)data;
 context.resetDeadline=deadline;context.resetLastTime=HandshakeTime(NULL);
 sNativeRenderActive=true;
 NativeGpu::RcsRecovery reset;
 NativeGpu::RcsRecovery::IO io={data,HandshakeTime,NativeRenderResetSafe,
  NativeRenderRawRead,NativeRenderRawWrite,HandshakePause};
 bool quiescent=reset.Run(io)==NativeGpu::RcsRecovery::Ok;
 sRenderReport.reset_stage=reset.LastReport().stage;sRenderReport.reset_error=reset.LastReport().error;
 return quiescent && NativeRenderResetSafe(data);
}
static bool NativeRenderResumeHardware()
{
 mutex_lock(&sLock);
 if(!sNativeRenderInitialized){mutex_unlock(&sLock);return true;}
 bool allowed=sNativeTransitionBusy && sNativeLifecycle.Current()==NativeGpu::RequestLifecycle::Suspended
  && !gKernelShutdown && !sObservationBlocked && sObservationPhase==ObservationIdle
  && (!sGroupAttempted || sNativeGroupReusable) && NativeRenderOwnerLocked()
  && sRenderOperationSerial!=UINT64_MAX;
 if(!allowed){mutex_unlock(&sLock);return false;}
 uint64_t operation=++sRenderOperationSerial;
 if(sRenderOwnership.Begin(sRenderDisplayLease,sRenderHandoff,operation)!=NativeGpu::RcsOwnership::Accepted){
  mutex_unlock(&sLock);return false;
 }
 sRenderActiveOperation=operation;
 sRenderReport={};sRenderReport.version=1;sRenderReport.mode=NATIVE_GPU_RENDER_BOOTSTRAP;
 mutex_unlock(&sLock);
 bool retired=false;
 struct ResumeScope {
  uint64_t operation;bool& retired;
  ~ResumeScope(){mutex_lock(&sLock);sNativeRenderActive=false;
   sRenderOwnership.Finish(sRenderDisplayLease,sRenderHandoff,operation,retired);
   mutex_unlock(&sLock);}
 } scope{operation,retired};
 IntelN100PciSnapshot original={};
 sRenderReport.pci_error=NativeRenderCapturePci(original);
 if(sRenderReport.pci_error!=IntelN100PciPower::Success || !NativeDmaStillDirect())return false;
 mutex_lock(&sLock);
 if(sObservationBlocked || sObservationPhase!=ObservationIdle
  || !NativeRenderOwnerLocked(true)){mutex_unlock(&sLock);return false;}
 sGroup.~IntelN100ForcewakeGroup();new (&sGroup) IntelN100ForcewakeGroup;
 sNativeGroupReusable=false;sGroupAttempted=true;sObservationPhase=ObservationHandshaking;
 sObservationLease=sMappingLease;sObservationSnapshot=original;sNativeExecutionActive=true;
 mutex_unlock(&sLock);
 uint32 mediaFuse=0;
 HandshakeContext h={sDevice,original,sMappingLease,sResources.registers,NULL,&mediaFuse,false,0,0};
 IntelN100Forcewake::IO bootstrap={&h,HandshakeTime,HandshakeRead,HandshakeWrite,
  HandshakePciStable,HandshakeLive,HandshakePause};
 GroupContext context={&h,{},0,0,{}};
 if(sForcewake.RunWithInspection(bootstrap,InspectInventory)==IntelN100Forcewake::Ok
  && NativeOwnerPower(context)
  && intel_n100_reset_domains(original.identity,uint8_t(original.classRevision),mediaFuse,context.plan)){
  IntelN100ForcewakeGroup::IO group={&context,HandshakeTime,NativeRenderGroupSafe,GroupRead,GroupWrite,HandshakePause};
  retired=sGroup.Run(group,original.identity,uint8_t(original.classRevision),mediaFuse,NativeRenderResumeWork)
   ==IntelN100ForcewakeGroup::Ok && NativeOwnerPower(context) && NativeDmaStillDirect();
 }
 mutex_lock(&sLock);
 if(retired)retired=NativeRenderOwnerLocked(true);
 sNativeGroupReusable=retired;
 if(retired){
  sRenderArena.RetireOrphans(true);
  // Only reset-quiescence + normal release permits rebuilding software ledgers.
  // Keep the whole GGTT/PPGTT pool; a new explicit bootstrap recreates defaults.
  sNativeRenderInitialized=false;sRenderDefaultValid=false;sRenderRetired=false;
  sRenderPublished=false;sRenderCursor=0;
  sRenderEngine.~RcsEngine();new (&sRenderEngine) NativeGpu::RcsEngine;
  sRenderCsb.~IntelN100CsbReader();new (&sRenderCsb) IntelN100CsbReader;
  sObservationPhase=ObservationIdle;sObservationLease=0;
 }else{sObservationPhase=ObservationUnknown;sObservationBlocked=true;}
 NativeRenderSnapshotLocked(sRenderReport);
 mutex_unlock(&sLock);return retired;
}
static bool NativeResourceAllowedLocked()
{
 return sRenderArenaReady && sRenderRetired && sRenderDefaultValid
  && sNativeRenderInitialized && sNativeLifecycle.Current()==NativeGpu::RequestLifecycle::Ready
  && !sObservationBlocked && NativeRenderOwnerLocked() && NativeDmaStillDirect();
}
static status_t NativeResourceOpen(uint64_t* session)
{
 if(!session)return B_BAD_VALUE;
 mutex_lock(&sLock);
 bool ok=NativeResourceAllowedLocked() && sRenderArena.Open(*session);
 if(!ok)dprintf("native RCS resource open refused arena=%u retired=%u default=%u init=%u lifecycle=%u blocked=%u owner=%u dma=%u\n",
  uint32_t(sRenderArenaReady),uint32_t(sRenderRetired),uint32_t(sRenderDefaultValid),
  uint32_t(sNativeRenderInitialized),uint32_t(sNativeLifecycle.Current()),
  uint32_t(sObservationBlocked),uint32_t(NativeRenderOwnerLocked()),uint32_t(NativeDmaStillDirect()));
 mutex_unlock(&sLock);return ok?B_OK:B_DEV_NOT_READY;
}
static status_t NativeResourceClose(uint64_t session)
{
 mutex_lock(&sLock);
 bool retired=NativeResourceAllowedLocked();
 bool ok=sRenderArena.Close(session,retired);
 mutex_unlock(&sLock);return ok?B_OK:B_BAD_VALUE;
}
static status_t NativeResourceCreate(uint64_t session,uint64_t bytes,uint32_t* handle,
 uint64_t* gpu,uint64_t* actual)
{
 if(!handle || !gpu || !actual)return B_BAD_VALUE;
 mutex_lock(&sLock);
 bool allowed=NativeResourceAllowedLocked();
 bool ok=allowed && sRenderArena.Create(session,bytes,*handle,*gpu,*actual);
 if(ok){
  unsigned first=unsigned((*gpu-kRenderArenaGpu)/4096),count=unsigned(*actual/4096);
  for(unsigned i=0;i<count;i++)memset(sRenderArenaPages[first+i].cpu,0,4096);
 }
 mutex_unlock(&sLock);return ok?B_OK:allowed?B_BAD_VALUE:B_DEV_NOT_READY;
}
static status_t NativeResourceTransfer(uint64_t session,bool write,uint32_t handle,
 uint64_t offset,uint8_t* bytes,uint32_t count)
{
 if(!bytes || !count || count>4096)return B_BAD_VALUE;
 mutex_lock(&sLock);
 uint64_t gpu=0;
 bool allowed=NativeResourceAllowedLocked();
 bool ok=allowed && sRenderArena.Resolve(session,handle,offset,count,gpu);
 if(ok && !write)ok=NativeVisible(NULL);
 if(ok){
  unsigned copied=0;
  while(copied<count){
   uint64_t relative=gpu-kRenderArenaGpu;
   unsigned page=unsigned(relative/4096),within=unsigned(relative&4095);
   unsigned chunk=count-copied<4096-within?count-copied:4096-within;
   uint8_t* cpu=(uint8_t*)sRenderArenaPages[page].cpu+within;
   if(write)memcpy(cpu,bytes+copied,chunk);
   else memcpy(bytes+copied,cpu,chunk);
   gpu+=chunk;copied+=chunk;
  }
 }
 mutex_unlock(&sLock);return ok?B_OK:allowed?B_BAD_VALUE:B_DEV_NOT_READY;
}
static status_t NativeResourceDestroy(uint64_t session,uint32_t handle)
{
 mutex_lock(&sLock);
 bool allowed=NativeResourceAllowedLocked();
 bool ok=allowed && sRenderArena.Destroy(session,handle,true);
 mutex_unlock(&sLock);return ok?B_OK:allowed?B_BAD_VALUE:B_DEV_NOT_READY;
}
static native_gpu_render_module sNativeRenderModule={
 {NATIVE_GPU_RENDER_MODULE,B_KEEP_LOADED,ObservationOperations},RunNativeRender,NativeRcsAcknowledgeDisplay
};
static native_gpu_render_large_module sNativeRenderLargeModule={
 {NATIVE_GPU_RENDER_LARGE_MODULE,B_KEEP_LOADED,ObservationOperations},RunNativeRenderLarge
};
static native_gpu_resource_module sNativeResourceModule={
 {NATIVE_GPU_RESOURCE_MODULE,B_KEEP_LOADED,ObservationOperations},NativeResourceOpen,
 NativeResourceClose,NativeResourceCreate,NativeResourceTransfer,NativeResourceDestroy,
 NativeResourceDraw
};
