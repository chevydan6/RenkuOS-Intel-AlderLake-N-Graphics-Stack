// SPDX-License-Identifier: MIT
#include <ACPI.h>
#include <kernel/lock.h>
#include "IntelDmarInventory.h"
#include "dmar-native-protocol.h"
// Bounded boot-lifetime retention: ACPI public ABI lacks put_table. Never
// repeat get_table. Normal DMAR lifetime was reviewed for this pinned kernel;
// raw ACPICA termination/uninstall from an unrelated module is outside contract.
static acpi_module_info* sAcpi;
static void* sRetainedTable;
static mutex sLock;
static bool sDone;
static DmarSnapshot sSnapshot;
static status_t Snapshot(DmarSnapshot* output)
{
	if(!output) return B_BAD_VALUE;
	mutex_lock(&sLock);
	if(!sDone) {
		sDone=true;
		sSnapshot.version=1;
		sSnapshot.status=sAcpi->get_table("DMAR",0,&sRetainedTable);
		if(sSnapshot.status==B_OK) {
			if(!sRetainedTable) sSnapshot.status=B_BAD_VALUE;
			else {
				const uint8_t* table=(const uint8_t*)sRetainedTable;
				uint32 length=dmar_u32(table+4);
				if(length<48 || length>sizeof(sSnapshot.bytes)) sSnapshot.status=B_NOT_SUPPORTED;
				else {
					memcpy(sSnapshot.bytes,table,length);
					sSnapshot.length=length;
					IntelDmarInventory inventory;
					if(!intel_dmar_inventory(sSnapshot.bytes,length,&inventory)) sSnapshot.status=B_BAD_DATA;
				}
			}
		}
	}
	*output=sSnapshot;
	mutex_unlock(&sLock);
	return B_OK;
}
static status_t Operations(int32 op,...)
{
	if(op==B_MODULE_INIT) {
		status_t status=get_module(B_ACPI_MODULE_NAME,(module_info**)&sAcpi);
		if(status!=B_OK) return status;
		mutex_init(&sLock,"DMAR boot snapshot");
		return B_OK;
	}
	return op==B_MODULE_UNINIT?B_BUSY:B_BAD_VALUE;
}
static DmarModule sModule={{DMAR_MODULE,B_KEEP_LOADED,Operations},Snapshot};
module_info* modules[]={(module_info*)&sModule,nullptr};
