/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <linux/dma-mapping.h>
#include <native_services.h>
#include "bus_dma_bridge.h"
#include "native_test.h"
int32 api_version = B_CUR_DRIVER_API_VERSION;
static sem_id sRun = -1;
static unsigned char sDeviceIdentity;
#define CHECK(r,c) do { (r)->checks++; if (!(c)) (r)->failures++; } while (0)
static void Run(struct linux_dma_result *r)
{
	struct device *device = (struct device *)&sDeviceIdentity;
	CHECK(r, dma_set_mask_and_coherent(device, DMA_BIT_MASK(39)) == 0);
	dma_addr_t address = 0xfeedface;
	CHECK(r, dma_alloc_coherent(device, 4096, &address, GFP_ATOMIC) == NULL);
	CHECK(r, address == 0xfeedface);
	CHECK(r, dma_mapping_error(device, DMA_MAPPING_ERROR) == -ENOMEM);
	const size_t sizes[] = {4096, 8192, 12288, 65536};
	for (unsigned index = 0; index < 4; index++) {
		size_t size = sizes[index];
		void *cpu = dma_alloc_coherent(device, size, &address, GFP_KERNEL);
		CHECK(r, cpu != NULL);
		if (!cpu) continue;
		r->coherent_allocations++;
		CHECK(r, address <= DMA_BIT_MASK(39) && size - 1 <= DMA_BIT_MASK(39) - address);
		CHECK(r, (uintptr_t)cpu != address);
		size_t alignment = 4096;
		while (alignment < size) alignment *= 2;
		CHECK(r, ((uintptr_t)cpu & (alignment - 1)) == 0);
		CHECK(r, (address & (alignment - 1)) == 0);
		CHECK(r, renku_bus_dma_forget(device) != 0);
		CHECK(r, dma_set_mask_and_coherent(device, DMA_BIT_MASK(32)) != 0);
		CHECK(r, renku_bus_dma_free(device, size + 1, cpu, address) != 0);
		volatile unsigned *words = cpu;
		for (size_t i = 0; i < size / 4; i++) CHECK(r, words[i] == 0);
		for (size_t i = 0; i < size / 4; i++) words[i] = 0xd00d0000u ^ i;
		for (size_t i = 0; i < size / 4; i++) CHECK(r, words[i] == (0xd00d0000u ^ i));
		dma_free_coherent(device, size, cpu, address);
		r->coherent_frees++;
	}
	struct renku_kpi_page backing[4] = {};
	struct page pages[4] = {};
	unsigned created = 0;
	for (; created < 4; created++) {
		status_t status = renku_kpi_page_create(&backing[created]);
		CHECK(r, status == B_OK);
		if (status != B_OK) break;
		pages[created].virtual = backing[created].cpu;
		pages[created].physical = backing[created].physical;
	}
	if (created == 4) {
		struct scatterlist first[3], last[2];
		sg_init_table(first, 3); sg_init_table(last, 2);
		sg_set_page(&first[0], &pages[0], 4096, 0);
		sg_set_page(&first[1], &pages[1], 1024, 128);
		sg_chain(first, 3, last);
		sg_set_page(&last[0], &pages[2], 2048, 0);
		sg_set_page(&last[1], &pages[3], 4096, 0);
		const unsigned long attrs = DMA_ATTR_SKIP_CPU_SYNC |
			DMA_ATTR_NO_KERNEL_MAPPING | DMA_ATTR_NO_WARN;
		unsigned count = dma_map_sg_attrs(device, first, 4, DMA_BIDIRECTIONAL, attrs);
		CHECK(r, count == 4);
		if (count) {
			r->streaming_maps += count;
			struct scatterlist *sg = first;
			for (unsigned i = 0; i < count; i++, sg = sg_next(sg)) {
				CHECK(r, sg != NULL && sg_dma_len(sg) == sg->length);
				CHECK(r, sg_dma_address(sg) <= DMA_BIT_MASK(39));
			}
			CHECK(r, renku_bus_dma_forget(device) != 0);
			CHECK(r, dma_map_sg_attrs(device, first, 4, DMA_BIDIRECTIONAL, attrs) == 0);
			dma_unmap_sg_attrs(device, first, 4, DMA_BIDIRECTIONAL, attrs);
			r->streaming_unmaps += count;
			CHECK(r, sg_dma_len(first) == 0);
		}
		CHECK(r, dma_map_sg_attrs(device, first, 4, DMA_NONE, attrs) == 0);
		/* The first map succeeds; the second entry fails. Native maps must
		 * roll back, permitting a subsequent device release. */
		first[1].length = 0;
		CHECK(r, dma_map_sg_attrs(device, first, 4, DMA_BIDIRECTIONAL, attrs) == 0);
		CHECK(r, sg_dma_len(first) == 0);
	}
	for (unsigned i = 0; i < created; i++)
		CHECK(r, renku_kpi_page_destroy(&backing[i]) == B_OK);
	CHECK(r, renku_bus_dma_forget(device) == 0);
}
static status_t Open(const char *name, uint32 flags, void **cookie)
{ (void)name; (void)flags; *cookie = NULL; return B_OK; }
static status_t Close(void *cookie) { (void)cookie; return B_OK; }
static status_t Control(void *cookie, uint32 operation, void *buffer, size_t length)
{
	(void)cookie;
	if (operation != LINUX_DMA_IOCTL || length != sizeof(struct linux_dma_result)) return B_BAD_VALUE;
	status_t status = acquire_sem(sRun);
	if (status != B_OK) return status;
	struct linux_dma_result result = {.version = 1};
	Run(&result);
	status = user_memcpy(buffer, &result, sizeof(result));
	release_sem(sRun); return status;
}
static device_hooks hooks = {Open, Close, Close, Control, NULL, NULL, NULL, NULL, NULL, NULL};
status_t init_hardware(void) { return B_OK; }
status_t init_driver(void)
{
	int status = renku_bus_dma_init();
	if (status != 0) return status;
	sRun = create_sem(1, "i915 Linux DMA test");
	if (sRun < B_OK) { renku_bus_dma_uninit(); return sRun; }
	dprintf("I915_LINUX_DMA_INIT status=0\n"); return B_OK;
}
void uninit_driver(void)
{
	int status = renku_bus_dma_uninit();
	if (status != 0) panic("Linux DMA test unloaded with live mappings: %d", status);
	status = delete_sem(sRun);
	dprintf("I915_LINUX_DMA_UNINIT status=%d\n", status);
}
const char **publish_devices(void)
{ static const char *names[] = {"misc/i915_linux_dma_test", NULL}; return names; }
device_hooks *find_device(const char *name)
{ return strcmp(name, "misc/i915_linux_dma_test") == 0 ? &hooks : NULL; }
