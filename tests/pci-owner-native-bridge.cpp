// SPDX-License-Identifier: MIT
#include <Drivers.h>
#include <IntelMemoryOwner.h>
#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>
#include "pci-owner-native-protocol.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;
static status_t Open(const char*, uint32, void** cookie)
{
	*cookie = NULL;
	return B_OK;
}
static status_t Close(void*) { return B_OK; }

static status_t
Control(void*, uint32 operation, void* buffer, size_t size)
{
	if (operation != PCI_OWNER_IOCTL || size != sizeof(PciOwnerResult))
		return B_BAD_VALUE;
	PciOwnerResult* result = (PciOwnerResult*)calloc(1, sizeof(PciOwnerResult));
	if (result == NULL)
		return B_NO_MEMORY;
	result->version = 1;
	intel_memory_observation_module_info* module = NULL;
	result->module = get_module(INTEL_MEMORY_OBSERVATION_MODULE_NAME,
		(module_info**)&module);
	if (result->module == B_OK) {
		for (unsigned i = 0; i < 2; i++) {
			PciOwnerRun& run = result->runs[i];
			run.begin = module->begin_observation(&run.lease, &run.snapshot);
			if (run.begin != B_OK)
				break;
			uint64 duplicate = 0;
			IntelN100PciSnapshot ignored = {};
			run.duplicate = module->begin_observation(&duplicate, &ignored);
			// End every admitted lease before any user copy. No lease, raw BDF,
			// pointer or MMIO input is accepted from the client.
			if (run.duplicate == B_OK)
				module->end_observation(duplicate);
			run.end = module->end_observation(run.lease);
			run.stale = module->end_observation(run.lease);
			if (run.duplicate != B_BUSY || run.end != B_OK
				|| run.stale != B_BAD_VALUE)
				break;
			result->completed++;
		}
		put_module(INTEL_MEMORY_OBSERVATION_MODULE_NAME);
	}
	status_t status = user_memcpy(buffer, result, sizeof(*result));
	free(result);
	return status;
}

static device_hooks sHooks = {
	Open, Close, Close, Control, NULL, NULL, NULL, NULL, NULL, NULL
};
extern "C" status_t init_hardware() { return B_OK; }
extern "C" status_t init_driver() { return B_OK; }
extern "C" void uninit_driver() {}
extern "C" const char** publish_devices()
{
	static const char* names[] = { "misc/intel_pci_owner", NULL };
	return names;
}
extern "C" device_hooks* find_device(const char* name)
{
	return strcmp(name, "misc/intel_pci_owner") == 0 ? &sHooks : NULL;
}
