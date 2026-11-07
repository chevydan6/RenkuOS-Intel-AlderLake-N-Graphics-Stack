// SPDX-License-Identifier: MIT
#include <KernelExport.h>
#include <kernel/lock.h>
#include "IntelDmarInventory.h"
#include "dmar-native-protocol.h"
#include "vtd-native-protocol.h"
static DmarModule* sDmar;
static DmarSnapshot sTable;
static VtdResult sResult;
static area_id sMappings[16];
static bool sDone;
static mutex sLock;
static bool Read(void* context,uint32 offset,uint32_t& value)
{
	// Narrow adapter: no generic register ranges and no store operation.
	if(offset!=0&&offset!=8&&offset!=12&&offset!=16&&offset!=20&&offset!=0x1c)
		return false;
	value=*(const volatile uint32*)((const uint8*)context+offset);
	return true;
}
static status_t Observe(VtdResult* output)
{
	if(!output) return B_BAD_VALUE;
	mutex_lock(&sLock);
	if(!sDone) {
		sDone=true;sResult.version=1;sResult.status=B_ERROR;
		IntelDmarInventory inventory;
		status_t status=sDmar->snapshot(&sTable);
		if(status==B_OK&&sTable.status==B_OK
			&&intel_dmar_inventory(sTable.bytes,sTable.length,&inventory)&&inventory.unitCount>0) {
			bool valid=B_PAGE_SIZE==4096&&sizeof(void*)==8;
			// Validate the entire plan before any mapping or register read.
			for(unsigned i=0;i<inventory.unitCount;i++) {
				uint64 base=inventory.units[i].registers;
				if(!base||(base&4095)||base>UINT64_MAX-4095) valid=false;
				for(unsigned j=0;j<i;j++) if(base==inventory.units[j].registers) valid=false;
			}
			if(valid) {
				sResult.count=inventory.unitCount;sResult.status=B_OK;
				for(unsigned i=0;i<sResult.count;i++) {
					sResult.units[i].base=inventory.units[i].registers;
					sResult.units[i].mapping=B_ERROR;
					sResult.units[i].reading=B_ERROR;
					sResult.units[i].cleanup=B_ERROR;
				}
				for(unsigned i=0;i<sResult.count;i++) {
					VtdUnitResult& unit=sResult.units[i];unit.base=inventory.units[i].registers;
					unit.reading=B_ERROR;unit.cleanup=B_ERROR;
					void* address=nullptr;
					sMappings[i]=map_physical_memory("VTd observer UC read",unit.base,4096,
						B_ANY_KERNEL_ADDRESS|B_UNCACHED_MEMORY,B_KERNEL_READ_AREA,&address);
					unit.mapping=sMappings[i]<B_OK?sMappings[i]:B_OK;
					if(unit.mapping!=B_OK) {sResult.status=unit.mapping;break;}
					uintptr_t mapped=reinterpret_cast<uintptr_t>(address);
					if(mapped&&!(mapped&4095)&&mapped<=UINTPTR_MAX-4095) {
						unit.reading=intel_vtd_sample(Read,address,unit.sample)?B_OK:B_ERROR;
						if(unit.reading==B_OK) unit.translation=intel_vtd_translation(unit.sample);
					}
					unit.cleanup=delete_area(sMappings[i]);
					if(unit.cleanup==B_OK) sMappings[i]=-1;
					if(unit.reading!=B_OK||unit.cleanup!=B_OK||unit.translation==IntelVtdUnknown) {
						sResult.status=B_ERROR;break;
					}
				}
			}
		}
	}
	*output=sResult;
	mutex_unlock(&sLock);return B_OK;
}
static status_t Operations(int32 op,...)
{
	if(op==B_MODULE_INIT) {
		status_t status=get_module(DMAR_MODULE,(module_info**)&sDmar);
		if(status!=B_OK)return status;
		for(unsigned i=0;i<16;i++)sMappings[i]=-1;
		mutex_init(&sLock,"VTd read observer");return B_OK;
	}
	return op==B_MODULE_UNINIT?B_BUSY:B_BAD_VALUE;
}
static VtdModule sModule={{VTD_MODULE,B_KEEP_LOADED,Operations},Observe};
module_info* modules[]={(module_info*)&sModule,nullptr};
