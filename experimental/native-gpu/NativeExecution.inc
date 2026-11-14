// SPDX-License-Identifier: MIT
// Copyright © 2014, 2016 Intel Corporation
// Native integration Copyright (c) 2026 chevydan6
// Native fixed-workload integration; included inside IntelMemoryOwner.cpp after
// its mapping helpers. All code, ledgers, mappings and callbacks stay in that
// B_KEEP_LOADED image. No pointer into an unloadable bridge is retained.
#include <ACPI.h>
#include <new>
#include <IntelDmarReserved.h>
#include <IntelVtdObservation.h>
#include <IntelN100PpgttTree.h>
#include <native_page_bridge.h>
#include "ExecutionModule.h"
#include "CopyRequestImage.h"
#include "BcsEngine.h"
#include "NativePciPower.h"
#include "OwnedBuffers.h"

static native_gpu_copy sNativeCopy;
static uint32_t sNativeExpected[1024];
static bool sNativeAttempted, sNativePageHold;
static bool sNativeReady, sNativeInitialized, sNativeRetired;
static uint32_t sNativeSequence, sNativeCursor;
// Busy/poisoned is set before touching any request backing. Only the complete
// retirement + power-release path clears it. Failures retain every allocation.
static native_gpu_execution_report sNativeReport;
static IntelN100PpgttTree sNativeTree;
static NativeGpu::BcsEngine sNativeEngine;
static IntelN100CsbReader sNativeCsb;
static acpi_module_info* sNativeAcpi;
static void* sNativeDmarTable;
static uint8 sNativeDmar[4096];
static IntelDmarReserved sNativeReserved;
static IntelDmarInventory sNativeInventory;
static area_id sNativeDmaAreas[16];
static addr_t sNativeDmaRegisters[16];
static uint32_t* sNativeSource;
static uint32_t* sNativeDestination;
static bool NativeOwnerPower(GroupContext& context)
{
	auto& h=*context.handshake;
	auto& s=context.safety;s.expected=h.original;s.observed={};
	if(!native_gpu_pci_power(ReadConfig,&h.device,h.original,s.observed)){
		s.pciResult=IntelN100PciPower::Unstable;s.reason=IntelN100ResetSafety::PciChanged;return false;
	}
	s.pciResult=IntelN100PciPower::Success;
	mutex_lock(&sLock);
	bool mapping=sObservationPhase==ObservationHandshaking && sObservationLease==h.lease
		&& sRegistersUncached && sResources.registers==h.registers;
	bool owner=sState.DiagnosticAllowed();mutex_unlock(&sLock);
	if(!mapping || !owner){s.reason=!mapping?IntelN100ResetSafety::Mapping:IntelN100ResetSafety::Owner;return false;}
	return true;
}

// This initial controlled-boot backend admits CPU physical identity ONLY when
// every enumerated remapping unit currently has translation disabled. It never
// disables VT-d or invents an identity map for an enabled IOMMU. Rechecked while
// working, not inferred from the historical observer's cached result.
static bool NativeDmaStillDirect()
{
	for (unsigned i=0;i<sNativeInventory.unitCount;i++) {
		if (!sNativeDmaRegisters[i]) return false;
		memory_read_barrier();
		uint32 status=*(const volatile uint32*)(sNativeDmaRegisters[i]+0x1c);
		memory_read_barrier();
		if (status==UINT32_MAX || (status & 0x80000000)) return false;
	}
	return sNativeInventory.unitCount!=0;
}
static bool NativeVtdRead(void* p,uint32_t offset,uint32_t& value)
{
	if (offset!=0 && offset!=8 && offset!=12 && offset!=16 && offset!=20 && offset!=0x1c)
		return false;
	memory_read_barrier();
	value=*(const volatile uint32*)((addr_t)p+offset);
	memory_read_barrier();
	return value!=UINT32_MAX;
}
static status_t NativeDmaAdmit()
{
	status_t status=get_module(B_ACPI_MODULE_NAME,(module_info**)&sNativeAcpi);
	if (status!=B_OK) return status;
	status=sNativeAcpi->get_table("DMAR",0,&sNativeDmarTable);
	if (status!=B_OK || !sNativeDmarTable) return status!=B_OK?status:B_BAD_DATA;
	// Public ACPI ABI has no put_table: retain one boot-lifetime reference.
	uint32 length=dmar_u32((const uint8*)sNativeDmarTable+4);
	if (length<48 || length>sizeof(sNativeDmar)) return B_NOT_SUPPORTED;
	memcpy(sNativeDmar,sNativeDmarTable,length);
	if (!intel_dmar_inventory(sNativeDmar,length,&sNativeInventory)
		|| !intel_dmar_reserved(sNativeDmar,length,&sNativeReserved)
		|| !sNativeInventory.unitCount) return B_BAD_DATA;
	bool gpuCovered=false;
	for (unsigned i=0;i<sNativeInventory.unitCount;i++) {
		const auto& unit=sNativeInventory.units[i];
		if (unit.segment!=0 || !unit.registers || (unit.registers&4095)
			|| unit.registers>UINT64_MAX-4095) return B_NOT_SUPPORTED;
		for (unsigned j=0;j<i;j++)
			if (unit.registers==sNativeInventory.units[j].registers) return B_BAD_DATA;
		gpuCovered |= (unit.flags&1)!=0;
		for (unsigned offset=unit.offset+16;offset<unit.offset+unit.length;) {
			const uint8* scope=sNativeDmar+offset;
			if (scope[0]==1 && scope[1]==8 && scope[5]==0 && scope[6]==2 && scope[7]==0)
				gpuCovered=true;
			offset+=scope[1];
		}
	}
	if (!gpuCovered || sDevice.bus!=0 || sDevice.device!=2 || sDevice.function!=0)
		return B_NOT_SUPPORTED;
	for (unsigned i=0;i<sNativeInventory.unitCount;i++) {
		void* address=NULL;
		sNativeDmaAreas[i]=map_physical_memory("native GPU VTd status",
			sNativeInventory.units[i].registers,4096,B_ANY_KERNEL_ADDRESS|B_UNCACHED_MEMORY,
			B_KERNEL_READ_AREA,&address);
		if (sNativeDmaAreas[i]<B_OK) return sNativeDmaAreas[i];
		sNativeDmaRegisters[i]=(addr_t)address;
		if (!address || ((addr_t)address&4095) || (addr_t)address>UINTPTR_MAX-4095)
			return B_BAD_DATA;
		IntelVtdSample sample={};
		if (!intel_vtd_sample(NativeVtdRead,address,sample)
			|| intel_vtd_translation(sample)!=IntelVtdDisabled) return B_NOT_SUPPORTED;
	}
	return NativeDmaStillDirect()?B_OK:B_NOT_SUPPORTED;
}
static bool NativeHold(void*) { if(sNativePageHold)return false;sNativePageHold=true;return true; }
static void NativeDrop(void*) { sNativePageHold=false; }
static bool NativeAllocate(void*,IntelN100PpgttTree::Page& page)
{
	void* cpu=NULL;uint64_t physical=0;
	int32_t area=renku_vm_page_run_create(4096,&cpu,&physical);
	if(area<B_OK)return false;
	page={uint64_t(uint32_t(area))+1,(uint64_t*)cpu,physical};
	// Invalid output remains recorded by the tree; never lose ownership.
	if(!intel_dmar_page_allowed(sNativeReserved,physical))page.dma=UINT64_MAX;
	return true;
}
static bool NativeFree(void*,uint64_t token)
{ return renku_vm_page_run_delete(int32_t(uint32_t(token-1)))==B_OK; }
static bool NativeOffset(GroupContext& context,uint32_t offset)
{
	bool allowed=(offset>=0x22000 && offset<=0x22550)
		|| (offset>=0x4000 && offset<=0x40fc)
		|| (offset>=0x4800 && offset<=0x481c)
		|| (offset>=0xb020 && offset<=0xb09c)
		|| offset==0xcee4 || offset==0xceec || offset==0xcec4;
	return allowed && !(offset&3) && uint64(offset)+4<=context.handshake->device.size
		&& context.handshake->registers<=UINTPTR_MAX-offset-3;
}
static bool NativeRead(void* data,uint32_t offset,uint32_t& value)
{
	auto& context=*(GroupContext*)data;
	if(!NativeOffset(context,offset))return false;
	memory_read_barrier();
	value=*(const volatile uint32*)(context.handshake->registers+offset);
	memory_read_barrier();
	return value!=UINT32_MAX;
}
static bool NativeWrite(void* data,uint32_t offset,uint32_t value)
{
	auto& context=*(GroupContext*)data;
	if(!NativeOffset(context,offset))return false;
	memory_write_barrier();
	*(volatile uint32*)(context.handshake->registers+offset)=value;
	memory_full_barrier();
	return true;
}
static bool NativeSafe(void* data)
{
	if(!ResetSafe(data) || !NativeDmaStillDirect())return false;
	uint32_t error=0,fault=0;
	if(!NativeRead(data,0x220b0,error) || !NativeRead(data,0xcec4,fault))return false;
	sNativeReport.error_register=error;sNativeReport.fault_register=fault;
	return error==0 && !(fault&1);
}
static bool NativeVisible(void*)
{
	// WB page tables/data and WC GART aliases share the GPU-owned backing.
	// Match the native GART's x86 flush contract before publication; after GPU
	// completion this also invalidates CPU copies before checking BLT output.
	asm volatile("mfence; wbinvd; mfence" ::: "memory");
	return true;
}
static bool NativeStoreChecked(void* data,uint32_t offset,uint32_t value)
{ return NativeSafe(data) && NativeWrite(data,offset,value) && NativeSafe(data); }
static bool NativeCachePolicy(void* data)
{
	// Linux v6.6 intel_mocs.c::gen12_mocs_table (ADL-P/ADL-N), defaults index2.
	// Intel copyright 2016; MIT. Global control and packed L3 tables are distinct.
	uint32_t control[64],l3[64];
	for(unsigned i=0;i<64;i++){control[i]=0x37;l3[i]=0x30;}
	const uint32_t c[]={0x37,5,5,0x37,0x17,0x17,0x27,0x27,0x77,0x77,
		0x57,0x57,0x67,0x67,0x4005,0x4005,0x60037,0x737,0x337,0x137,0x3b7,0x7b7};
	for(unsigned i=0;i<22;i++){control[i+2]=c[i];l3[i+2]=(i==0 || i>=16 || (i&1))?0x30:0x10;}
	// Entries3..5 precede the repeating UC/WB pairs at6..17.
	l3[3]=l3[5]=0x10;l3[4]=0x30;
	control[48]=0x37;control[49]=5;control[50]=0x37;control[51]=5;
	l3[48]=l3[49]=0x30;l3[50]=l3[51]=0x10;
	control[60]=control[62]=control[63]=0x37;control[61]=5;
	l3[60]=l3[62]=l3[63]=0x10;l3[61]=0x30;
	for(unsigned i=0;i<64;i++)if(!NativeStoreChecked(data,0x4000+4*i,control[i]))return false;
	for(unsigned i=0;i<32;i++)if(!NativeStoreChecked(data,0xb020+4*i,l3[2*i]|(l3[2*i+1]<<16)))return false;
	const uint32_t pat[]={3,1,2,0,3,3,3,3}; // tgl_setup_private_ppat(), same ADL-N path.
	for(unsigned i=0;i<8;i++)if(!NativeStoreChecked(data,0x4800+4*i,pat[i]))return false;
	uint32_t value=0;
	if(!NativeRead(data,0x400c,value) || value!=5
		|| !NativeRead(data,0x480c,value) || value!=0)return false;
	// BCS0 uses an unmasked Gen12 invalidation request. Retain ADL-P OA WA.
	if(!NativeStoreChecked(data,0xcee4,1) || !NativeStoreChecked(data,0xceec,1))return false;
	for(unsigned tries=0;tries<128;tries++) {
		if(!NativeSafe(data) || !NativeRead(data,0xcee4,value))return false;
		if(!(value&1))return true;
		HandshakePause(data);
	}
	return false;
}
static bool NativeStatus32(void*,uint32_t offset,uint32_t& value)
{
	if(offset!=kN100CsbWrite)return false;
	uint32_t raw=*(const volatile uint32_t*)(sMappingSet.buffers[1].cpu_aperture+offset);
	sNativeReport.csb_tail_raw=raw;
	// Linux process_csb reads this field into u8; upper dword bits are not
	// part of its cursor. Keep the raw word for the actual hardware report.
	value=uint8_t(raw);
	return true;
}
static bool NativeStatus64(void*,uint32_t offset,uint64_t& value)
{
	if(offset<kN100CsbBuffer || offset>=kN100CsbBuffer+12*8 || (offset&7))return false;
	value=*(const volatile uint64_t*)(sMappingSet.buffers[1].cpu_aperture+offset);
	return true;
}
static bool NativeAcquire(void*) { memory_full_barrier();return true; }
static uint32_t NativeHash(const uint32_t* cpu)
{ uint32_t h=2166136261u;for(unsigned i=0;i<1024;i++)h=(h^cpu[i])*16777619u;return h; }
static bool NativeCollectOutput(void* data)
{
	if(!NativeSafe(data) || !NativeVisible(data))return false;
	auto& r=sNativeReport;
	const volatile uint32_t* result=(const volatile uint32_t*)sMappingSet.buffers[2].cpu_aperture;
	const volatile uint32_t* status=(const volatile uint32_t*)sMappingSet.buffers[1].cpu_aperture;
	r.observed_nonce=result[0];r.observed_complement=result[1];r.observed_sequence=status[0x40];
	r.csb_tail_raw=status[kN100CsbWrite/4];
	for(unsigned i=0;i<4;i++)NativeStatus64(NULL,kN100CsbBuffer+8*i,r.csb_words[i]);
	r.source_hash=NativeHash(sNativeSource);r.destination_hash=NativeHash(sNativeDestination);
	r.copy_matched=0;
	for(unsigned y=0;y<sNativeCopy.height;y++)
		for(unsigned x=0;x<sNativeCopy.width;x++)
			if(sNativeSource[y*(sNativeCopy.source_pitch/4)+x]
				==sNativeDestination[y*(sNativeCopy.destination_pitch/4)+x])r.copy_matched++;
	return NativeRead(data,0x2209c,r.mode);
}
static bool NativeGroupSafe(void* data)
{
	auto& context=*(GroupContext*)data;
	// Same checks as GroupSafe, using the existing diagnostic owner adapter so
	// an actual failed check preserves its PCI/mapping/owner reason and values.
	if(!ResetOwnerPower(context))return false;
	return true;
}
static bool NativeWork(void* data,uint64_t deadline)
{
	auto& context=*(GroupContext*)data;
	sNativeReport.stage=6;
	context.resetDeadline=deadline;context.resetLastTime=HandshakeTime(NULL);
	if(!sNativeInitialized && !ResetWork(data,deadline))return false;
	sNativeReport.reset=B_OK;
	if(!NativeSafe(data))return false;
	sNativeReport.stage=7;
	uint64_t root=0;if(!sNativeTree.Root(root))return false;
	auto& h=*context.handshake;
	NativeGpu::ImageInput in={h.original.identity,uint32_t(uint8_t(h.original.classRevision)),{},
		root,sNativeReport.nonce,sNativeReport.sequence,
		1+(sNativeReport.sequence-1)%0x7fe};
	for(unsigned i=0;i<4;i++)in.ggtt[i]=sMappingSet.buffers[i].ggtt_offset;
	NativeGpu::ImageBuffers cpu={(uint32_t*)sMappingSet.buffers[0].cpu_aperture,
		(uint32_t*)sMappingSet.buffers[1].cpu_aperture,(uint32_t*)sMappingSet.buffers[2].cpu_aperture,
		(uint32_t*)sMappingSet.buffers[3].cpu_aperture};
	NativeGpu::ImageOutput image;
	if(!NativeGpu::BuildCopyRequestImage(in,cpu,image,!sNativeInitialized,sNativeCopy.width,
		sNativeCopy.height,sNativeCopy.source_pitch,sNativeCopy.destination_pitch))return false;
	// The HWSP cursor may become visible before its entry write. Poison only
	// the next two consumed/free slots so wraparound cannot expose old tags.
	for(unsigned i=1;i<=2;i++) {
		unsigned slot=(sNativeCursor+i)%kN100CsbEntries;
		cpu.status[(kN100CsbBuffer+8*slot)/4]=UINT32_MAX;
		cpu.status[(kN100CsbBuffer+8*slot)/4+1]=UINT32_MAX;
	}
	sNativeReport.ring_tail=image.tail;
	sNativeReport.stage=8;
	if(!NativeVisible(data) || (!sNativeInitialized && !NativeCachePolicy(data)))return false;
	sNativeReport.stage=9;
	// Explicitly align native MMIO/software/HWSP cursors at0. This leaves
	// slots1/2 for the observed N100 activation/idle records; slot0 stays poison.
	if(!sNativeInitialized)cpu.status[kN100CsbWrite/4]=0;
	NativeGpu::BcsEngine::IO io={data,HandshakeTime,NativeSafe,NativeRead,NativeWrite,NativeVisible};
	if(!sNativeInitialized) {
		if(!sNativeEngine.Initialize(io,in.identity,in.revision,in.ggtt[1],deadline,0))return false;
		sNativeInitialized=true;
	} else if(!sNativeRetired || !sNativeEngine.Rearm(io,deadline))return false;
	sNativeRetired=false;
	sNativeReport.stage=10;
	if(!sNativeEngine.Publish(io,image.descriptor))return false;
	sNativeReport.engine=B_OK;
	sNativeReport.stage=11;
	IntelN100CsbReader::Io csb={data,HandshakeTime,NativeSafe,NativeStatus32,
		NativeStatus64,NativeAcquire,HandshakePause};
	uint64_t now=HandshakeTime(NULL);
	if(now>=deadline || !sNativeCsb.Run(csb,sMappingLease,in.tag,deadline-now,2048,sNativeCursor)){
		NativeCollectOutput(data);
		return false;
	}
	sNativeReport.csb=B_OK;
	sNativeReport.activation=sNativeCsb.LastReport().events.raw[0];
	sNativeReport.completion=sNativeCsb.LastReport().events.raw[1];
	sNativeReport.stage=12;
	if(!NativeCollectOutput(data))return false;
	if(sNativeReport.observed_nonce!=in.nonce || sNativeReport.observed_complement!=~in.nonce
		|| sNativeReport.observed_sequence!=in.sequence)return false;
	sNativeReport.stage=13;
	if(sNativeReport.copy_matched!=sNativeReport.copy_words)return false;
	// Verify untouched destination bytes as well as the copied rectangle.
	for(unsigned i=0;i<1024;i++)if(sNativeDestination[i]!=sNativeExpected[i])return false;
	uint32_t mode=0;
	if(!NativeRead(data,0x2209c,mode) || !(mode&0x200) || !NativeSafe(data))return false;
	sNativeReport.mode=mode;
	// CSB switched to idle, matching post-flush breadcrumb and complete data,
	// then engine-idle/error/owner checks: no GPU access to this context remains.
	// Keep its mappings pinned, and preserve the consumed CSB tail (i915 process_csb software head).
	sNativeCursor=sNativeCsb.LastReport().tail;
	sNativeRetired=true;
	sNativeReport.stage=14;
	return true;
}
static status_t RunNativeCopy(const native_gpu_copy* copy,const uint32_t* source,
	uint32_t* destination,native_gpu_execution_report* output)
{
	if(!output)return B_BAD_VALUE;
	*output={};output->version=1;output->status=B_BAD_VALUE;
	if(copy && (!source || !destination || source==destination
		|| !NativeGpu::ValidCopy(copy->width,copy->height,copy->source_pitch,copy->destination_pitch)))
		return B_BAD_VALUE;
	mutex_lock(&sLock);
	if(sNativeAttempted){mutex_unlock(&sLock);return B_BUSY;}
	if(sNativeSequence==UINT32_MAX){mutex_unlock(&sLock);return B_BUSY;}
	sNativeAttempted=true;
	sNativeCopy=copy?*copy:native_gpu_copy{1,2,1024,1,4096,4096,{}};
	++sNativeSequence;
	if(sNativeSequence==NativeGpu::kPoison)++sNativeSequence;
	sNativeReport={};sNativeReport.version=1;sNativeReport.status=B_ERROR;
	sNativeReport.mapping=sNativeReport.dma=sNativeReport.tables=sNativeReport.bootstrap
		=sNativeReport.group=sNativeReport.reset=sNativeReport.engine=sNativeReport.csb=B_ERROR;
	mutex_unlock(&sLock);
	auto& r=sNativeReport;
	r.stage=1;
	if(!sNativeReady) {
	r.mapping=BeginMappingSet(&sMappingSet);
	if(r.mapping!=B_OK){*output=r;return r.status;}
	// This is the execution extension of mapping/v1. Its original cleanup API
	// must never reclaim objects that this path can publish. One retained set
	// is reused only after proven retirement; failure retains it until reboot.
	mutex_lock(&sLock);sMappingRetained=true;r.retained=1;mutex_unlock(&sLock);
	r.stage=2;r.dma=NativeDmaAdmit();
	if(r.dma!=B_OK){*output=r;return r.status;}
	r.stage=3;
	IntelN100PpgttTree::Backend backend={NULL,NativeHold,NativeDrop,NativeAllocate,NativeFree};
	if(!sNativeTree.Build(backend,0x200000,2,3,3,IntelN100DirectoryUncached)){
		*output=r;return r.status;
	}
	r.tables=B_OK;r.pages=sNativeTree.RecordedPages();
	IntelN100PpgttTree::Page page={};
	if(!sNativeTree.Describe(8,page)){*output=r;return r.status;}
	sNativeSource=(uint32_t*)page.cpu;
	if(!sNativeTree.Describe(9,page)){*output=r;return r.status;}
	sNativeDestination=(uint32_t*)page.cpu;
	sNativeReady=true;
	}
	r.mapping=r.dma=r.tables=B_OK;r.pages=sNativeTree.RecordedPages();r.retained=1;
	r.nonce=(uint32_t(system_time())^sNativeSequence)|1;
	if(r.nonce==NativeGpu::kPoison || ~r.nonce==NativeGpu::kPoison)r.nonce^=0x915;
	r.sequence=sNativeSequence;r.copy_words=sNativeCopy.width*sNativeCopy.height;
	// CPU -> GPU: copy only from kernel-owned, session-locked staging. The
	// publication path flushes WB data and WC request stores with NativeVisible.
	for(unsigned i=0;i<1024;i++){
		sNativeSource[i]=copy?source[i]:r.nonce^(0x9e3779b9u*i);
		sNativeDestination[i]=copy?destination[i]:NativeGpu::kPoison;
		sNativeExpected[i]=sNativeDestination[i];
	}
	for(unsigned y=0;y<sNativeCopy.height;y++)
		for(unsigned x=0;x<sNativeCopy.width;x++)
			sNativeExpected[y*(sNativeCopy.destination_pitch/4)+x]=sNativeSource[y*(sNativeCopy.source_pitch/4)+x];
	r.stage=4;
	IntelN100PciSnapshot original={};
	if(IntelN100PciPower::Capture(ReadConfig,&sDevice,sDevice.base,sDevice.size,original)
		!=IntelN100PciPower::Success){*output=r;return r.status;}
	r.identity=original.identity;r.revision=uint8_t(original.classRevision);
	mutex_lock(&sLock);
	if(sObservationBlocked || sObservationPhase!=ObservationIdle || (sGroupAttempted && !sNativeRetired)
		|| !sState.DiagnosticAllowed()) {mutex_unlock(&sLock);*output=r;return B_BUSY;}
	// Previous group is recreated only after its successful release and retirement.
	if(sNativeRetired){sGroup.~IntelN100ForcewakeGroup();new (&sGroup) IntelN100ForcewakeGroup;}
	sNativeCsb.~IntelN100CsbReader();new (&sNativeCsb) IntelN100CsbReader;
	sGroupAttempted=true;sObservationPhase=ObservationHandshaking;
	sObservationLease=sMappingLease;sObservationSnapshot=original;
	sNativeExecutionActive=true;
	mutex_unlock(&sLock);
	uint32 mediaFuse=0;
	HandshakeContext h={sDevice,original,sMappingLease,sResources.registers,NULL,&mediaFuse,false,0,0};
	IntelN100Forcewake::IO io={&h,HandshakeTime,HandshakeRead,HandshakeWrite,
		HandshakePciStable,HandshakeLive,HandshakePause};
	r.stage=5;
	r.bootstrap=sForcewake.RunWithInspection(io,InspectInventory)==IntelN100Forcewake::Ok?B_OK:B_ERROR;
	bool success=false;
	GroupContext context={&h,{},0,0,{}};
	if(r.bootstrap==B_OK && NativeOwnerPower(context)
		&& intel_n100_reset_domains(original.identity,uint8(original.classRevision),mediaFuse,context.plan)) {
		IntelN100ForcewakeGroup::IO groupIO={&context,HandshakeTime,NativeGroupSafe,GroupRead,GroupWrite,HandshakePause};
		r.group=sGroup.Run(groupIO,original.identity,uint8(original.classRevision),mediaFuse,NativeWork)
			==IntelN100ForcewakeGroup::Ok?B_OK:B_ERROR;
		success=r.group==B_OK && NativeOwnerPower(context) && NativeDmaStillDirect();
	}
	r.bootstrap_stage=sForcewake.LastReport().stage;r.bootstrap_error=sForcewake.LastReport().error;
	r.group_stage=sGroup.LastReport().stage;r.group_error=sGroup.LastReport().error;
	r.reset_stage=sBcsReset.LastReport().stage;r.reset_error=sBcsReset.LastReport().error;
	r.safety_reason=context.safety.reason;r.safety_phase=context.safety.phase;r.safety_domain=context.safety.domain;
	r.pci_result=context.safety.pciResult;
	r.expected_command=context.safety.expected.command;r.observed_command=context.safety.observed.command;
	r.expected_status=context.safety.expected.status;r.observed_status=context.safety.observed.status;
	r.expected_pm=context.safety.expected.pmControl;r.observed_pm=context.safety.observed.pmControl;
	r.group_domain=sGroup.LastReport().domain;
	for(unsigned i=0;i<sGroup.LastReport().count;i++){
		r.group_reads+=sGroup.LastReport().domains[i].reads;
		r.group_writes+=sGroup.LastReport().domains[i].writes;
	}
	r.engine_stage=sNativeEngine.LastReport().stage;r.engine_error=sNativeEngine.LastReport().error;
	r.load_attempted=sNativeEngine.LastReport().loadAttempted;r.csb_error=sNativeCsb.LastReport().error;
	mutex_lock(&sLock);
	if(success){sObservationPhase=ObservationIdle;sObservationLease=0;r.status=B_OK;}
	else{sObservationPhase=ObservationUnknown;sObservationBlocked=true;}
	// Never RetireObservation(): this transaction uses the mapping lease/pin,
	// not a second observation lease. Both request backing and code stay alive.
	*output=r;
	status_t status=r.status;
	if(success){
		// GPU -> CPU: NativeCollectOutput invalidated CPU caches after ordered
		// completion. Only full retirement + power release exposes new data.
		if(copy)memcpy(destination,sNativeDestination,kNativeBufferBytes);
		sNativeAttempted=false;
	}
	mutex_unlock(&sLock);
	return status;
}
static status_t RunNativeExecution(native_gpu_execution_report* output)
{ return RunNativeCopy(NULL,NULL,NULL,output); }
static native_gpu_execution_module sNativeExecutionModule={
	{NATIVE_GPU_EXECUTION_MODULE,B_KEEP_LOADED,ObservationOperations},RunNativeExecution,RunNativeCopy
};
