// SPDX-License-Identifier: MIT
#ifndef INTEL_DMAR_INVENTORY_H
#define INTEL_DMAR_INVENTORY_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Pure inventory parser, not PCI scope resolution or DMA admission.
struct IntelDmarInventory {
	struct Unit { uint64_t registers; uint16_t segment, offset, length;
		uint8_t flags, scopes; };
	Unit units[16];
	unsigned unitCount, records, unknownRecords, unknownScopes;
};
static inline uint16_t dmar_u16(const uint8_t* b) { return b[0] | (uint16_t(b[1]) << 8); }
static inline uint32_t dmar_u32(const uint8_t* b) { return dmar_u16(b) | (uint32_t(dmar_u16(b+2)) << 16); }
static inline uint64_t dmar_u64(const uint8_t* b) { return dmar_u32(b) | (uint64_t(dmar_u32(b+4)) << 32); }
static inline bool
intel_dmar_inventory(const uint8_t* data, size_t size, IntelDmarInventory* output)
{
	if (!data || !output || size < 48 || size > 4096
		|| memcmp(data,"DMAR",4) != 0 || dmar_u32(data+4) != size)
		return false;
	uint8_t sum=0;
	for(size_t i=0;i<size;i++) sum+=data[i];
	if(sum) return false;
	IntelDmarInventory result={};
	for(size_t offset=48;offset<size;) {
		if(size-offset<4) return false;
		uint16_t type=dmar_u16(data+offset), length=dmar_u16(data+offset+2);
		if(length<4 || length>size-offset) return false;
		result.records++;
		if(type==0) {
			if(length<16 || result.unitCount==16) return false;
			IntelDmarInventory::Unit& unit=result.units[result.unitCount++];
			unit={dmar_u64(data+offset+8),dmar_u16(data+offset+6),
				(uint16_t)offset,length,data[offset+4],0};
			for(size_t scope=offset+16;scope<offset+length;) {
				if(offset+length-scope<6) return false;
				unsigned scopeLength=data[scope+1];
				if(scopeLength<6 || scopeLength>offset+length-scope || (scopeLength-6)%2)
					return false;
				unsigned scopeType=data[scope];
				if(scopeType<1 || scopeType>4) result.unknownScopes++;
				else {
					if(scopeLength<8) return false;
					for(unsigned path=6;path<scopeLength;path+=2)
						if(data[scope+path]>31 || data[scope+path+1]>7) return false;
				}
				if(unit.scopes==255) return false;
				unit.scopes++;
				scope+=scopeLength;
			}
		} else result.unknownRecords++;
		offset+=length;
	}
	*output=result;
	return true;
}
#endif
