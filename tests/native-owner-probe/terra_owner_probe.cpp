// SPDX-License-Identifier: MIT
// Disposable-VM test driver. It never matches or accesses GPU PCI hardware.
#include <Drivers.h>
#include <KernelExport.h>
#include <IntelMemoryOwner.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;
struct ProbeCookie { area_id area; uint32* data; };

static status_t probe_open(const char*, uint32, void** result)
{
	ProbeCookie* cookie = (ProbeCookie*)malloc(sizeof(ProbeCookie));
	if (cookie == NULL)
		return B_NO_MEMORY;
	void* address = NULL;
	cookie->area = create_area("terra owner RAM surrogate", &address,
		B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA);
	if (cookie->area < B_OK) {
		status_t status = cookie->area;
		free(cookie);
		return status;
	}
	cookie->data = (uint32*)address;
	cookie->data[0] = TERRA_PROBE_SENTINEL;
	*result = cookie;
	return B_OK;
}
static status_t probe_close(void*) { return B_OK; }
static status_t probe_free(void* data)
{
	ProbeCookie* cookie = (ProbeCookie*)data;
	status_t status = delete_area(cookie->area);
	free(cookie);
	return status;
}
static status_t probe_control(void* data, uint32 opcode, void* buffer, size_t length)
{
	ProbeCookie* cookie = (ProbeCookie*)data;
	if (opcode == TERRA_PROBE_CHECK)
		return cookie->data[0] == TERRA_PROBE_USER_VALUE ? B_OK : B_ERROR;
	if (opcode != TERRA_PROBE_GET || length < sizeof(terra_probe_result))
		return B_BAD_VALUE;
	terra_probe_result result = {};
	result.area = cookie->area;
	result.lease = 0x517a6e21;
	intel_memory_owner_resources resources;
	memset(&resources, 0xa5, sizeof(resources));
	intel_memory_owner_resources before;
	memcpy(&before, &resources, sizeof(before));
	intel_memory_owner_module_info* owner = NULL;
	result.module_status = get_module(INTEL_MEMORY_OWNER_MODULE_NAME,
		(module_info**)&owner);
	result.begin_status = B_ERROR;
	if (result.module_status == B_OK) {
		// Device/function 255 are outside the PCI encoding range, so this
		// cannot select a real enumerated device or map its BAR.
		result.begin_status = owner->begin_display(255, 255, 255,
			&result.lease, &resources);
		put_module(INTEL_MEMORY_OWNER_MODULE_NAME);
	}
	result.outputs_unchanged = result.lease == 0x517a6e21
		&& memcmp(&resources, &before, sizeof(resources)) == 0;
	return user_memcpy(buffer, &result, sizeof(result));
}
static device_hooks kHooks = {
	probe_open, probe_close, probe_free, probe_control,
	NULL, NULL, NULL, NULL, NULL, NULL
};
extern "C" status_t init_hardware() { return B_OK; }
extern "C" status_t init_driver() { return B_OK; }
extern "C" void uninit_driver() {}
extern "C" const char** publish_devices()
{
	static const char* names[] = { "misc/terra_owner_probe", NULL };
	return names;
}
extern "C" device_hooks* find_device(const char* name)
{
	return strcmp(name, "misc/terra_owner_probe") == 0 ? &kHooks : NULL;
}
