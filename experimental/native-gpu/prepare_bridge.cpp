// SPDX-License-Identifier: MIT
#include <Drivers.h>
#include <KernelExport.h>
#include <string.h>
#include "PrepareModule.h"
int32 api_version = B_CUR_DRIVER_API_VERSION;
static status_t Open(const char*, uint32, void** cookie) { *cookie = NULL; return B_OK; }
static status_t Close(void*) { return B_OK; }
static status_t Control(void*, uint32 operation, void* buffer, size_t bytes)
{
	if (operation != NATIVE_GPU_PREPARE_IOCTL || bytes != sizeof(native_gpu_prepare_report))
		return B_BAD_VALUE;
	native_gpu_prepare_module* module = NULL;
	status_t status = get_module(NATIVE_GPU_PREPARE_MODULE, (module_info**)&module);
	if (status != B_OK) return status;
	native_gpu_prepare_report report = {};
	status = module->prepare(&report);
	put_module(NATIVE_GPU_PREPARE_MODULE);
	status_t copied = user_memcpy(buffer, &report, sizeof(report));
	return copied == B_OK ? status : copied;
}
static device_hooks hooks = {Open, Close, Close, Control, NULL, NULL, NULL, NULL, NULL, NULL};
extern "C" status_t init_hardware() { return B_OK; }
extern "C" status_t init_driver() { dprintf("NATIVE_GPU_PREPARE_BRIDGE_INIT\n"); return B_OK; }
extern "C" void uninit_driver() { dprintf("NATIVE_GPU_PREPARE_BRIDGE_UNINIT\n"); }
extern "C" const char** publish_devices()
{
	static const char* names[] = {"misc/intel_native_prepare", NULL};
	return names;
}
extern "C" device_hooks* find_device(const char* name)
{
	return strcmp(name, "misc/intel_native_prepare") == 0 ? &hooks : NULL;
}
