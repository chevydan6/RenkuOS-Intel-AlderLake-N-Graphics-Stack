// SPDX-License-Identifier: MIT
#include <Drivers.h>
#include <IntelMemoryOwner.h>
#include <KernelExport.h>
#include <string.h>
#include "forcewake-native-protocol.h"
int32 api_version = B_CUR_DRIVER_API_VERSION;
static status_t Open(const char*, uint32, void** cookie)
{
	*cookie = NULL; return B_OK;
}
static status_t Close(void*) { return B_OK; }
static status_t Control(void*, uint32 operation, void* buffer, size_t size)
{
	if (operation != FORCEWAKE_IOCTL || size != sizeof(ForcewakeResult))
		return B_BAD_VALUE;
	ForcewakeResult result = {};
	result.version = 1; result.transaction = B_ERROR;
	intel_gt_handshake_module_info* module = NULL;
	result.module = get_module(INTEL_GT_HANDSHAKE_MODULE_NAME, (module_info**)&module);
	if (result.module == B_OK) {
		// Exactly one transaction. Unknown ownership remains in the owner;
		// closing this test device cannot reclaim its resources or pins.
		result.transaction = module->run_handshake(&result.report);
		put_module(INTEL_GT_HANDSHAKE_MODULE_NAME);
	}
	return user_memcpy(buffer, &result, sizeof(result));
}
static device_hooks sHooks = { Open, Close, Close, Control, NULL, NULL, NULL, NULL, NULL, NULL };
extern "C" status_t init_hardware() { return B_OK; }
extern "C" status_t init_driver() { return B_OK; }
extern "C" void uninit_driver() {}
extern "C" const char** publish_devices()
{
	static const char* names[] = { "misc/intel_forcewake", NULL }; return names;
}
extern "C" device_hooks* find_device(const char* name)
{
	return strcmp(name, "misc/intel_forcewake") == 0 ? &sHooks : NULL;
}
