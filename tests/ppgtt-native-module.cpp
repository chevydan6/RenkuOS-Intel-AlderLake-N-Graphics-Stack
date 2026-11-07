// SPDX-License-Identifier: MIT
// Resident memory-only test. No PCI, MMIO or GPU command access.
#include <KernelExport.h>
#include <kernel/lock.h>
#include "IntelN100KernelPages.h"
#include "ppgtt-native-protocol.h"
#include "dmar-native-protocol.h"
static DmarModule* sDmar;
static DmarSnapshot sFirmware;
static mutex sLock;
static IntelN100KernelPages sProvider;
static IntelN100PpgttTree sTree;
static PpgttTestResult sResult;
static bool sRan;

static bool Verify(uint64 window, unsigned count)
{
	IntelN100PpgttTree::Page pages[24];
	for (unsigned i=0;i<8+count;i++) {
		if (!sTree.Describe(i,pages[i])) return false;
		// Independent physical exclusion oracle: decode raw firmware records,
		// rather than calling the provider policy's page-allowed helper.
		for(size_t offset=48;offset<sFirmware.length;
			offset+=dmar_u16(sFirmware.bytes+offset+2)) {
			if(dmar_u16(sFirmware.bytes+offset)!=1)continue;
			uint64 base=dmar_u64(sFirmware.bytes+offset+8);
			uint64 limit=dmar_u64(sFirmware.bytes+offset+16);
			if(pages[i].dma<=limit&&base<=pages[i].dma+4095)return false;
		}
		sResult.reserved_checked++;
		area_info info;
		if (get_area_info(area_for(pages[i].cpu),&info)!=B_OK
			|| info.address!=pages[i].cpu || info.size!=4096 || info.lock!=B_FULL_LOCK)
			return false;
	}
	unsigned indices[]={unsigned((window>>39)&511),unsigned((window>>30)&511),unsigned((window>>21)&511)};
	for (unsigned page=0;page<7;page++) {
		for (unsigned slot=0;slot<512;slot++) {
			uint64 expected;
			if (page<3) expected=pages[slot==indices[page]?page+1:page+4].dma|3;
			else if (page==3) expected=pages[slot<count?8+slot:7].dma|(slot<count?3:0x1b);
			else if (page<6) expected=pages[page+1].dma|3;
			else expected=pages[7].dma|0x1b;
			if (pages[page].cpu[slot]!=expected) return false;
		}
	}
	for(unsigned page=7;page<8+count;page++)
		for(unsigned byte=0;byte<4096;byte++)
			if (((uint8*)pages[page].cpu)[byte]!=0) return false;
	uint64 root=0;
	if(!sTree.Root(root)||root!=pages[0].dma) return false;
	for(unsigned n=0;n<=512;n++) {
		uint64 va=n<512?window+n*4096:window^(1ULL<<39);
		uint64 physical=root;
		for(unsigned shift=39;;shift-=9) {
			unsigned page=0;
			while(page<7 && pages[page].dma!=physical) page++;
			if(page==7) return false;
			physical=pages[page].cpu[(va>>shift)&511]&~4095ULL;
			if(shift==12) break;
		}
		if(physical!=pages[n<count?8+n:7].dma) return false;
	}
	return true;
}

static status_t Run(PpgttTestResult* output)
{
	if(!output) return B_BAD_VALUE;
	mutex_lock(&sLock);
	if(!sRan) {
		sRan=true;
		sResult.version=2; sResult.status=B_ERROR;
		bool configured=sDmar->snapshot(&sFirmware)==B_OK&&sFirmware.version==1
			&&sFirmware.status==B_OK&&sProvider.ConfigureReserved(sFirmware.bytes,sFirmware.length);
		sResult.reserved_ranges=sProvider.ReservedRanges();
		for(unsigned test=0;configured&&test<3;test++) {
			sResult.current_case=test; sResult.stage=1;
			unsigned count=test==1?16:1;
			uint64 window=test==2?(1ULL<<48)-(1ULL<<21):0;
			if(!sTree.Build(sProvider.Backend(),window,count,0,3,IntelN100DirectoryCached)) break;
			sResult.stage=2;
			area_id ids[24]; bool valid=true;
			for(unsigned i=0;i<8+count;i++) {
				IntelN100PpgttTree::Page page;
				if(!sTree.Describe(i,page)||(ids[i]=area_for(page.cpu))<B_OK) {valid=false;break;}
			}
			if(!valid) break;
			sResult.stage=3;
			if(!Verify(window,count)) break;
			sResult.verified_areas+=8+count;
			sResult.stage=4;
			if(!sTree.Release()||sProvider.LiveAreas()!=0) break;
			sResult.stage=5;
			for(unsigned i=0;i<8+count;i++) {
				area_info info;
				if(get_area_info(ids[i],&info)!=B_OK) sResult.absent_areas++;
				// ID reuse is possible; this observation is supporting evidence.
			}
			sResult.completed++;
		}
		sResult.tree_state=sTree.CurrentState();
		sResult.live_areas=sProvider.LiveAreas();
		sResult.provider_error=sProvider.LastError();
		if(sResult.completed==3) sResult.status=B_OK;
	}
	*output=sResult;
	mutex_unlock(&sLock);
	return B_OK;
}
static status_t Operations(int32 op,...)
{
	if(op==B_MODULE_INIT) {
		status_t status=get_module(DMAR_MODULE,(module_info**)&sDmar);
		if(status!=B_OK)return status;
		mutex_init(&sLock,"ppgtt reserved test");return B_OK;
	}
	return op==B_MODULE_UNINIT?B_BUSY:B_BAD_VALUE;
}
static PpgttTestModule sModule={{PPGTT_TEST_MODULE,B_KEEP_LOADED,Operations},Run};
module_info* modules[]={(module_info*)&sModule,nullptr};
