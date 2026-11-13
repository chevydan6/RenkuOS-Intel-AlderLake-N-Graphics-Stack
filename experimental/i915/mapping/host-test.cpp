// SPDX-License-Identifier: MIT
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <new>
using uint64 = uint64_t;
using addr_t = uintptr_t;
using status_t = int;
using area_id = int;
constexpr unsigned B_ANY_KERNEL_ADDRESS=0, B_READ_AREA=1, B_WRITE_AREA=2,
 B_EXECUTE_AREA=4, B_KERNEL_READ_AREA=8, B_KERNEL_WRITE_AREA=16;
struct area_info { void* address; size_t size; unsigned protection; };
constexpr int B_OK=0, B_BUSY=-1, B_BAD_VALUE=-2, B_NO_MEMORY=-3, B_ERROR=-4;
constexpr int B_APERTURE_NON_RESERVED=1;
#include "IntelApertureLeaseState.h"
#include "mapping-api.inc"
static std::mutex sLock;
static IntelApertureLeaseState sState;
static bool sObservationBlocked;
static constexpr int ObservationIdle=0;
static int sObservationPhase;
static void mutex_lock(std::mutex* m) { m->lock(); }
static void mutex_unlock(std::mutex* m) { m->unlock(); }
static status_t Status(IntelApertureLeaseState::Result r)
{
	return r == IntelApertureLeaseState::Success ? B_OK
		: r == IntelApertureLeaseState::Busy ? B_BUSY : B_BAD_VALUE;
}
struct aperture_info { uint64 physical_base; addr_t base; size_t size, reserved_size; };
static int queryFailure, allocFailure, freeFailure, allocations, frees, live;
static bool reenter, closeDuringAllocation;
static unsigned badRange;
static uint64 display;
static aperture_info aperture;
static int cloneFailure, deleteFailure, aliases;
static bool userAlias;
static constexpr addr_t aliasBase=0x800000000ULL;
static area_id area_for(void* p) { assert((addr_t)p==aperture.base); return 10; }
static status_t get_area_info(area_id id, area_info* out)
{
 assert(id==10 || id==20);
 *out={reinterpret_cast<void*>(id==10 ? aperture.base : aliasBase), aperture.size,
  id==10 || userAlias ? B_READ_AREA|B_WRITE_AREA : B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA};
 return B_OK;
}
static area_id clone_area(const char*, void** out, unsigned spec, unsigned protection, area_id id)
{
 assert(id==10 && spec==B_ANY_KERNEL_ADDRESS);
 assert(protection==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA));
 if (cloneFailure) return B_ERROR;
 *out=reinterpret_cast<void*>(aliasBase); aliases++; return 20;
}
static status_t delete_area(area_id id)
{
 assert(id==20 && aliases==1);
 if (deleteFailure) return B_ERROR;
 aliases--; return B_OK;
}
static status_t Query(uint64, aperture_info* info)
{
	*info = aperture;
	return queryFailure ? B_ERROR : B_OK;
}
static status_t BeginMappingSet(intel_memory_mapping_set*);
static status_t EndMappingSet(uint64);
static status_t Allocate(uint64 lease, size_t, size_t alignment, uint32_t flags,
	addr_t* base, void*)
{
	assert(alignment==4096 && flags==B_APERTURE_NON_RESERVED);
	allocations++;
	if (reenter) {
		intel_memory_mapping_set out = {};
		assert(BeginMappingSet(&out)==B_BUSY);
		assert(EndMappingSet(lease)==B_BUSY);
	}
	if (closeDuringAllocation && allocations == 1)
		assert(sState.Release(display)==IntelApertureLeaseState::Success);
	if (allocations==allocFailure)
		return B_ERROR;
	*base=aperture.base+aperture.reserved_size+allocations*0x10000;
	if (badRange==1) *base=aperture.base-4096;
	if (badRange==2) *base=aperture.base;
	if (badRange==3) *base=aperture.base+aperture.size;
	if (badRange==4) (*base)++;
	if (badRange==5) *base=aperture.base+aperture.reserved_size;
	live++;
	return B_OK;
}
static status_t Free(uint64, addr_t base)
{
	assert(base >= aperture.base && base < aperture.base+aperture.size && live>0);
	frees++;
	if (frees==freeFailure) return B_ERROR;
	live--;
	return B_OK;
}
#include "mapping-under-test.inc"
static void Reset()
{
	sState.~IntelApertureLeaseState(); new(&sState) IntelApertureLeaseState;
	sMappingPin.~Operation(); new(&sMappingPin) IntelApertureLeaseState::Operation;
	sMappingLease=0; sMappingBusy=false; sMappingRetained=false; sMappingSet={};
	sObservationBlocked=false; sObservationPhase=ObservationIdle;
	queryFailure=allocFailure=freeFailure=allocations=frees=live=0;
 cloneFailure=deleteFailure=aliases=0; userAlias=false;
 sMappingAliasArea=-1; sMappingAliasBase=0;
	reenter=closeDuringAllocation=false; badRange=0;
	aperture={0x80000000,0x100000000ULL,0x1000000,0x100000};
	assert(sState.BeginInitialization(display)==IntelApertureLeaseState::Success);
	assert(sState.FinishInitialization(display,true)==IntelApertureLeaseState::Success);
}
static void FailedBegin()
{
	intel_memory_mapping_set output, before;
	memset(&output,0xa5,sizeof(output)); before=output;
	assert(BeginMappingSet(&output)!=B_OK);
	assert(memcmp(&output,&before,sizeof(output))==0);
	assert(sMappingLease!=0 && sMappingRetained);
	assert(BeginMappingSet(&output)==B_BUSY);
	assert(frees==0); // No guessed cleanup of ambiguous backing.
}
int main()
{
	Reset(); assert(BeginMappingSet(nullptr)==B_BAD_VALUE); assert(allocations==0);
	for (unsigned round=0;round<4;round++) {
		Reset(); reenter=true;
		intel_memory_mapping_set output={}, before={};
		assert(BeginMappingSet(&output)==B_OK);
		assert(output.token!=0 && live==4);
		assert(output.buffers[3].size==65536 && aliases==1);
  assert(output.buffers[0].cpu_aperture==aliasBase+output.buffers[0].ggtt_offset);
		assert(output.buffers[0].ggtt_offset==aperture.reserved_size+0x10000);
		assert(BeginMappingSet(&before)==B_BUSY);
		assert(EndMappingSet(output.token+1)==B_BAD_VALUE);
		assert(EndMappingSet(0)==B_BAD_VALUE);
		assert(EndMappingSet(output.token)==B_OK);
		assert(live==0 && frees==4 && sMappingLease==0 && aliases==0);
		assert(EndMappingSet(output.token)==B_BAD_VALUE);
		assert(BeginMappingSet(&output)==B_OK);
		assert(EndMappingSet(output.token)==B_OK);
	}
	for (int stage=1;stage<=4;stage++) {
		Reset(); allocFailure=stage; FailedBegin(); assert(live==stage-1);
		Reset(); freeFailure=stage;
		intel_memory_mapping_set output={};
		assert(BeginMappingSet(&output)==B_OK);
		assert(EndMappingSet(output.token)==B_ERROR);
		assert(live==5-stage && sMappingLease==output.token);
		freeFailure=0;
		assert(EndMappingSet(output.token)==B_OK);
		assert(live==0 && frees==5); // Exactly four successful frees, one failure.
	}
	Reset(); cloneFailure=1; FailedBegin(); assert(live==0);
 Reset(); userAlias=true; FailedBegin(); assert(live==0 && aliases==1);
 Reset(); deleteFailure=1; intel_memory_mapping_set aliasSet={};
 assert(BeginMappingSet(&aliasSet)==B_OK);
 assert(EndMappingSet(aliasSet.token)==B_ERROR && live==0 && aliases==1);
 deleteFailure=0;
 assert(EndMappingSet(aliasSet.token)==B_OK && aliases==0 && frees==4);
 Reset(); queryFailure=1; FailedBegin(); assert(allocations==0);
	for (unsigned range=1;range<=5;range++) { Reset(); badRange=range; FailedBegin(); }
	Reset(); aperture.size=4096; aperture.reserved_size=0; FailedBegin();
	Reset(); aperture.base=UINTPTR_MAX-4095; FailedBegin(); assert(allocations==0);
	Reset(); closeDuringAllocation=true; FailedBegin();
	Reset(); sObservationBlocked=true; intel_memory_mapping_set output={};
	assert(BeginMappingSet(&output)==B_BUSY && allocations==0);
	puts("PASS actual mapping adapter: ownership, overlap/bounds, all allocation/free failures,");
	puts("retry without double free, reentry, close overlap and unchanged failed outputs.");
}
