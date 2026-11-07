// SPDX-License-Identifier: MIT
#include <Drivers.h>
#include <KernelExport.h>
#include <string.h>
#include "ppgtt-native-protocol.h"
int32 api_version=B_CUR_DRIVER_API_VERSION;
static status_t Open(const char*,uint32,void** cookie) {*cookie=nullptr;return B_OK;}
static status_t Close(void*) {return B_OK;}
static status_t Control(void*,uint32 op,void* buffer,size_t size)
{
	if(op!=PPGTT_TEST_IOCTL||size!=sizeof(PpgttTestResult)) return B_BAD_VALUE;
	PpgttTestModule* module=nullptr;
	status_t status=get_module(PPGTT_TEST_MODULE,(module_info**)&module);
	if(status!=B_OK) return status;
	PpgttTestResult result={};
	status=module->run(&result);
	put_module(PPGTT_TEST_MODULE);
	return status==B_OK?user_memcpy(buffer,&result,sizeof(result)):status;
}
static device_hooks sHooks={Open,Close,Close,Control,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
extern "C" status_t init_hardware() {return B_OK;}
extern "C" status_t init_driver() {return B_OK;}
extern "C" void uninit_driver() {}
extern "C" const char** publish_devices() {static const char* names[]={"misc/intel_ppgtt_reserved_test",nullptr};return names;}
extern "C" device_hooks* find_device(const char* name) {return strcmp(name,"misc/intel_ppgtt_reserved_test")==0?&sHooks:nullptr;}
