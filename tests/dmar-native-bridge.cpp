// SPDX-License-Identifier: MIT
#include <Drivers.h>
#include <KernelExport.h>
#include <string.h>
#include <stdlib.h>
#include "dmar-native-protocol.h"
int32 api_version=B_CUR_DRIVER_API_VERSION;
static status_t Open(const char*,uint32,void** cookie) {*cookie=nullptr;return B_OK;}
static status_t Close(void*) {return B_OK;}
static status_t Control(void*,uint32 op,void* buffer,size_t size)
{
	if(op!=DMAR_IOCTL||size!=sizeof(DmarSnapshot)) return B_BAD_VALUE;
	DmarModule* module=nullptr;
	status_t status=get_module(DMAR_MODULE,(module_info**)&module);
	if(status!=B_OK) return status;
	DmarSnapshot* result=(DmarSnapshot*)malloc(sizeof(DmarSnapshot));
	if(!result) {put_module(DMAR_MODULE);return B_NO_MEMORY;}
	status=module->snapshot(result);
	put_module(DMAR_MODULE);
	if(status==B_OK) status=user_memcpy(buffer,result,sizeof(*result));
	free(result);
	return status;
}
static device_hooks sHooks={Open,Close,Close,Control,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
extern "C" status_t init_hardware() {return B_OK;}
extern "C" status_t init_driver() {return B_OK;}
extern "C" void uninit_driver() {}
extern "C" const char** publish_devices() {static const char* names[]={"misc/intel_dmar_inventory",nullptr};return names;}
extern "C" device_hooks* find_device(const char* name) {return strcmp(name,"misc/intel_dmar_inventory")==0?&sHooks:nullptr;}
