// SPDX-License-Identifier: MIT
#include "IntelN100KernelPages.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static void* sMemory[24];
static unsigned sCreated, sDeleted, sMaps;
static int sCreateFailure=-1, sMapFault=-1, sDeleteFailure=-1;
static bool sInvalidCpu;
static int sFaultPage=-1;
area_id create_area(const char*, void** address, unsigned where, size_t size,
	unsigned lock, unsigned protection)
{
	assert(where==B_ANY_KERNEL_ADDRESS && size==4096 && lock==B_FULL_LOCK);
	assert(protection==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA));
	if (int(sCreated)==sCreateFailure) return B_NO_MEMORY;
	assert(sCreated<24 && posix_memalign(&sMemory[sCreated],4096,4096)==0);
	memset(sMemory[sCreated],0xa5,4096);
	*address=sInvalidCpu ? (char*)sMemory[sCreated]+1 : sMemory[sCreated];
	return sCreated++;
}
status_t get_memory_map_etc(int team,const void* address,size_t size,
	physical_entry* entry,uint32* count)
{
	assert(team==B_CURRENT_TEAM && address && size==4096 && *count==1);
	unsigned page=sMaps++;
	*entry={0x100000+page*4096ULL,4096};
	if (sFaultPage>=0 && unsigned(sFaultPage)!=page) return B_OK;
	if (sMapFault==0) return B_BAD_ADDRESS;
	if (sMapFault==1) entry->size=4095;
	if (sMapFault==2) entry->address=1ULL<<39;
	if (sMapFault==3) entry->address++;
	if (sMapFault==4) *count=0;
	if (sMapFault==5) *count=2;
	return B_OK;
}
status_t delete_area(area_id area)
{
	assert(area>=0 && unsigned(area)<sCreated && sMemory[area]);
	if (area==sDeleteFailure) return B_BAD_ADDRESS;
	free(sMemory[area]); sMemory[area]=nullptr; sDeleted++; return B_OK;
}
static void Reset()
{
	// End of mock boot: only the test process reclaims quarantined test RAM.
	for(void*& memory:sMemory) { free(memory); memory=nullptr; }
	sCreated=sDeleted=sMaps=0;
	sCreateFailure=sMapFault=sDeleteFailure=-1;
	sInvalidCpu=false; sFaultPage=-1;
}

static void Seal(uint8_t* bytes,unsigned size)
{
	bytes[4]=size&255;bytes[5]=size>>8;bytes[9]=0;
	uint8_t sum=0;for(unsigned i=0;i<size;i++)sum+=bytes[i];bytes[9]=-sum;
}
static void Configure(IntelN100KernelPages& provider)
{
	uint8_t bytes[48]={};memcpy(bytes,"DMAR",4);Seal(bytes,48);
	assert(provider.ConfigureReserved(bytes,sizeof(bytes)));
}
static void Reservation(uint8_t* bytes,uint64_t base,uint64_t limit)
{
	memset(bytes,0,80);memcpy(bytes,"DMAR",4);bytes[48]=1;bytes[50]=32;
	for(unsigned i=0;i<8;i++){bytes[56+i]=base>>(i*8);bytes[64+i]=limit>>(i*8);}
	bytes[72]=1;bytes[73]=8;bytes[78]=2;Seal(bytes,80);
}
int main()
{
	unsigned cases=0;
	{
		IntelN100KernelPages provider; Configure(provider); IntelN100PpgttTree tree;
		assert(tree.Build(provider.Backend(),0,16,0,3,IntelN100DirectoryCached));
		assert(provider.LiveAreas()==24 && sMaps==24);
		assert(tree.Release() && provider.LiveAreas()==0 && sDeleted==24);
		assert(tree.Release()); cases++;
	}
	for(int fail=0;fail<24;fail++) {
		Reset(); sCreateFailure=fail;
		IntelN100KernelPages provider; Configure(provider); IntelN100PpgttTree tree;
		assert(!tree.Build(provider.Backend(),0,16,0,3,IntelN100DirectoryCached));
		assert(provider.LiveAreas()==0 && sDeleted==unsigned(fail));
		assert(tree.CurrentState()==IntelN100PpgttTree::Empty); cases++;
	}
	for(int fault=0;fault<5;fault++) {
		Reset(); sMapFault=fault;
		// Persistent records model residency until this mock boot ends.
		static IntelN100KernelPages providers[5]; static IntelN100PpgttTree trees[5];
		auto& provider=providers[fault]; Configure(provider); auto& tree=trees[fault];
		assert(!tree.Build(provider.Backend(),0,1,0,3,IntelN100DirectoryCached));
		assert(tree.CurrentState()==IntelN100PpgttTree::Ambiguous);
		assert(provider.LiveAreas()==1 && provider.LastError()!=B_OK && sDeleted==0);
		assert(!tree.Release() && sDeleted==0);
		cases++;
	}
	Reset(); sInvalidCpu=true;
	static IntelN100KernelPages invalidCpu; static IntelN100PpgttTree invalidTree;
	Configure(invalidCpu);
	assert(!invalidTree.Build(invalidCpu.Backend(),0,1,0,3,IntelN100DirectoryCached));
	assert(sMaps==0 && invalidCpu.LiveAreas()==1 && sDeleted==0); cases++;
	Reset(); sMapFault=5; sFaultPage=4;
	static IntelN100KernelPages late; static IntelN100PpgttTree lateTree;
	Configure(late);
	assert(!lateTree.Build(late.Backend(),0,1,0,3,IntelN100DirectoryCached));
	assert(late.LiveAreas()==5 && sDeleted==0 && sMaps==5); cases++;
	Reset(); sCreateFailure=5; sDeleteFailure=2;
	static IntelN100KernelPages rollback; static IntelN100PpgttTree rollbackTree;
	Configure(rollback);
	assert(!rollbackTree.Build(rollback.Backend(),0,1,0,3,IntelN100DirectoryCached));
	assert(rollbackTree.CurrentState()==IntelN100PpgttTree::Retained);
	assert(rollback.LiveAreas()==1 && sDeleted==4); cases++;
	Reset();
	{
		IntelN100KernelPages provider; Configure(provider);
		auto backend=provider.Backend();
		assert(backend.retain(backend.context));
		assert(!backend.retain(backend.context));
		backend.release(backend.context);
		assert(backend.retain(backend.context));
		backend.release(backend.context); cases++;
	}
	Reset(); sDeleteFailure=2;
	static IntelN100KernelPages retained; static IntelN100PpgttTree tree;
	Configure(retained);
	assert(tree.Build(retained.Backend(),0,1,0,3,IntelN100DirectoryCached));
	assert(!tree.Release() && retained.LiveAreas()==1 && sDeleted==8);
	sDeleteFailure=-1; // Unexpected deletion error stays sticky; do not reuse ID.
	assert(!tree.Release() && retained.LiveAreas()==1 && sDeleted==8); cases++;
	Reset();

	Reset();
	{
		IntelN100KernelPages provider;IntelN100PpgttTree candidate;
		assert(!candidate.Build(provider.Backend(),0,1,0,3,IntelN100DirectoryCached));
		assert(sCreated==0);cases++;
	}
	static IntelN100KernelPages blocked[2];static IntelN100PpgttTree blockedTrees[2];
	for(unsigned n=0;n<2;n++) {
		Reset();uint8_t bytes[80];uint64_t base=0x100000+(n?4:0)*4096;
		Reservation(bytes,base,base+4095);
		assert(blocked[n].ConfigureReserved(bytes,80));
		bytes[9]++;assert(!blocked[n].ConfigureReserved(bytes,80));
		assert(blocked[n].ReservedRanges()==1);
		assert(!blockedTrees[n].Build(blocked[n].Backend(),0,1,0,3,IntelN100DirectoryCached));
		assert(blockedTrees[n].CurrentState()==IntelN100PpgttTree::Ambiguous);
		assert(sCreated==(n?5:1)&&sDeleted==0&&blocked[n].LastError()==B_NOT_ALLOWED);
		for(unsigned i=0;i<sCreated;i++)for(unsigned j=0;j<4096;j++)assert(((uint8_t*)sMemory[i])[j]==0xa5);
		assert(!blockedTrees[n].Release());
		assert(!blocked[n].ConfigureReserved(bytes,80));cases++;
	}
	Reset();
	{
		IntelN100KernelPages provider;Configure(provider);auto backend=provider.Backend();
		assert(backend.retain(backend.context));uint8_t bytes[80];Reservation(bytes,0x100000,0x100fff);
		assert(!provider.ConfigureReserved(bytes,80));backend.release(backend.context);cases++;
	}
	printf("%u kernel-page adapter cases passed with mocked VM APIs.\n",cases);
}
