// SPDX-License-Identifier: MIT
#include <Drivers.h>
#include <IntelMemoryOwner.h>
#include <KernelExport.h>
#include <string.h>
#include "native_probe.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;
static sem_id sRun = -1;
static status_t Open(const char*, uint32, void** cookie)
{
	*cookie = NULL;
	return B_OK;
}
static status_t Close(void*) { return B_OK; }
static void Check(i915_mapping_result& result, bool passed)
{
	result.checks++;
	if (!passed)
		result.failures++;
}
static uint32 Pattern(unsigned slot, size_t word, bool reverse)
{
	uint32 value = 0x91500000u ^ (slot << 20) ^ (uint32)word;
	return reverse ? ~value : value;
}
static status_t Control(void*, uint32 operation, void* buffer, size_t size)
{
	if (operation != I915_MAPPING_TEST_IOCTL || size != sizeof(i915_mapping_result))
		return B_BAD_VALUE;
	status_t status = acquire_sem(sRun);
	if (status != B_OK)
		return status;
	i915_mapping_result result = {};
	result.version = 1;
	result.begin_status = result.end_status = B_ERROR;
	intel_memory_mapping_module_info* module = NULL;
	result.module_status = get_module(INTEL_MEMORY_MAPPING_MODULE_NAME,
		(module_info**)&module);
	if (result.module_status == B_OK) {
		intel_memory_mapping_set set = {};
		result.begin_status = module->begin_set(&set);
		if (result.begin_status == B_OK) {
			Check(result, set.token != 0);
			intel_memory_mapping_set unchanged;
			memset(&unchanged, 0xa5, sizeof(unchanged));
			intel_memory_mapping_set before = unchanged;
			Check(result, module->begin_set(&unchanged) == B_BUSY);
			Check(result, memcmp(&before, &unchanged, sizeof(before)) == 0);
			Check(result, module->end_set(0) == B_BAD_VALUE);
			const size_t sizes[] = {4096, 4096, 4096, 65536};
			bool valid = true;
			for (unsigned i = 0; i < 4; i++) {
				const auto& b = set.buffers[i];
				result.ggtt[i] = b.ggtt_offset;
				result.bytes[i] = b.size;
				bool bounds = b.cpu_aperture != 0 && b.size == sizes[i]
					&& (b.cpu_aperture & 4095) == 0 && (b.ggtt_offset & 4095) == 0;
				Check(result, bounds);
				valid = valid && bounds;
				for (unsigned j = 0; j < i; j++) {
					bool separate = b.ggtt_offset >= set.buffers[j].ggtt_offset
						+ set.buffers[j].size || set.buffers[j].ggtt_offset
						>= b.ggtt_offset + b.size;
					Check(result, separate);
					valid = valid && separate;
				}
			}
			if (valid) {
				for (unsigned pass = 0; pass < 2; pass++) {
					// CPU stores only. Do not use the future GPU store sentinel.
					for (unsigned i = 0; i < 4; i++) {
						volatile uint32* words = (volatile uint32*)set.buffers[i].cpu_aperture;
						for (size_t w = 0; w < set.buffers[i].size / 4; w++)
							words[w] = Pattern(i, w, pass != 0);
					}
					// The aperture is WC (or UC fallback). Drain WC CPU stores.
					asm volatile("sfence" ::: "memory");
					// Read all buffers after all writes: detects cross-buffer aliasing.
					for (unsigned i = 0; i < 4; i++) {
						volatile uint32* words = (volatile uint32*)set.buffers[i].cpu_aperture;
						for (size_t w = 0; w < set.buffers[i].size / 4; w++)
							Check(result, words[w] == Pattern(i, w, pass != 0));
					}
				}
			}
			// No device publication occurred; all CPU use has ended.
			result.end_status = module->end_set(set.token);
			if (result.end_status == B_OK)
				Check(result, module->end_set(set.token) == B_BAD_VALUE);
		}
		put_module(INTEL_MEMORY_MAPPING_MODULE_NAME);
	}
	status = user_memcpy(buffer, &result, sizeof(result));
	release_sem(sRun);
	return status;
}
static device_hooks sHooks = {Open, Close, Close, Control, NULL, NULL, NULL, NULL, NULL, NULL};
extern "C" status_t init_hardware() { return B_OK; }
extern "C" status_t init_driver()
{
	sRun = create_sem(1, "i915 mapping diagnostic");
	dprintf("I915_MAPPING_TEST_INIT status=%ld\n", (long)(sRun < 0 ? sRun : B_OK));
	return sRun < 0 ? sRun : B_OK;
}
extern "C" void uninit_driver()
{
	dprintf("I915_MAPPING_TEST_UNINIT status=%ld\n", (long)delete_sem(sRun));
}
extern "C" const char** publish_devices()
{
	static const char* names[] = {"misc/i915_mapping_test", NULL};
	return names;
}
extern "C" device_hooks* find_device(const char* name)
{
	return strcmp(name, "misc/i915_mapping_test") == 0 ? &sHooks : NULL;
}
