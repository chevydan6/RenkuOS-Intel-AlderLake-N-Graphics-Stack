// SPDX-License-Identifier: MIT
#pragma once
#include "IntelDmarInventory.h"
struct IntelDmarReserved {
	struct Range {uint64_t base,limit;}; // Inclusive firmware limits.
	Range ranges[16];
	unsigned count;
};
// Conservative: excludes every RMRR irrespective of segment/scope. This is an
// allocation filter, not PCI scope resolution or GPU DMA admission.
static inline bool
intel_dmar_reserved(const uint8_t* bytes,size_t size,IntelDmarReserved* output)
{
	IntelDmarInventory inventory;
	if(!output||!intel_dmar_inventory(bytes,size,&inventory)) return false;
	if(inventory.unknownScopes) return false;
	for(unsigned i=0;i<inventory.unitCount;i++)
		if(inventory.units[i].flags&~1u)return false;
	IntelDmarReserved result={};
	for(size_t offset=48;offset<size;) {
		unsigned type=dmar_u16(bytes+offset),length=dmar_u16(bytes+offset+2);
		if(type==1) {
			if(length<32||result.count==16||dmar_u16(bytes+offset+4)!=0) return false;
			uint64_t base=dmar_u64(bytes+offset+8),limit=dmar_u64(bytes+offset+16);
			if(base>limit||(base&4095)||(limit&4095)!=4095) return false;
			for(size_t scope=offset+24;scope<offset+length;) {
				if(offset+length-scope<8) return false;
				unsigned type=bytes[scope],n=bytes[scope+1];
				if(type<1||type>4||n<8||n>offset+length-scope||(n-6)%2) return false;
				for(unsigned path=6;path<n;path+=2)
					if(bytes[scope+path]>31||bytes[scope+path+1]>7)return false;
				scope+=n;
			}
			result.ranges[result.count++]={base,limit};
		} else if(type!=0) return false; // Unimplemented firmware structures stay unknown.
		offset+=length;
	}
	*output=result;return true;
}
static inline bool
intel_dmar_page_allowed(const IntelDmarReserved& reserved,uint64_t page)
{
	if((page&4095)||page>UINT64_MAX-4095||reserved.count>16)return false;
	for(unsigned i=0;i<reserved.count;i++)
		if(page<=reserved.ranges[i].limit&&reserved.ranges[i].base<=page+4095)return false;
	return true;
}
