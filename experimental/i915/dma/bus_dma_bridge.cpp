// SPDX-License-Identifier: MIT
// OS compatibility only: use RenkuOS's existing bus_dma implementation unchanged.
extern "C" {
#include <sys/malloc.h>
#include <sys/bus.h>
#include <machine/bus.h>
}
#include "bus_dma_bridge.h"
#include <limits.h>

struct Binding {
	const void* device;
	uint64_t mask;
	unsigned live;
};
struct Allocation {
	const void* device;
	bus_dma_tag_t tag;
	bus_dmamap_t map;
	void* cpu;
	void* backing;
	area_id alias_area;
	size_t size;
	uint64_t dma;
	bool owned, ready;
	const void* key;
	int direction;
	Allocation* next;
};
static sem_id sLock = -1;
static Binding sBindings[16];
static Allocation* sAllocations;
static status_t Lock()
{
	status_t s;
	do { s = acquire_sem(sLock); } while (s == B_INTERRUPTED);
	return s;
}
static Binding* Find(const void* device)
{
	for (auto& binding : sBindings)
		if (binding.device == device)
			return &binding;
	return NULL;
}
extern "C" int renku_bus_dma_init()
{
	if (sLock >= B_OK)
		return B_BUSY;
	sLock = create_sem(1, "i915 Linux DMA bindings");
	return sLock < B_OK ? sLock : B_OK;
}
extern "C" int renku_bus_dma_uninit()
{
	status_t s = Lock();
	if (s != B_OK) return s;
	for (const auto& binding : sBindings) {
		if (binding.device != NULL) {
			release_sem(sLock);
			return B_BUSY;
		}
	}
	release_sem(sLock);
	s = delete_sem(sLock);
	if (s == B_OK) sLock = -1;
	return s;
}
extern "C" int renku_bus_dma_set_mask(const void* device, uint64_t mask)
{
	if (device == NULL || mask < 4095 || (mask & (mask + 1)) != 0)
		return B_BAD_VALUE;
	status_t s = Lock();
	if (s != B_OK) return s;
	Binding* binding = Find(device);
	if (binding != NULL && binding->live != 0) {
		release_sem(sLock);
		return B_BUSY;
	}
	if (binding == NULL) {
		for (auto& candidate : sBindings)
			if (candidate.device == NULL) { binding = &candidate; break; }
	}
	if (binding != NULL) *binding = {device, mask, 0};
	release_sem(sLock);
	return binding != NULL ? B_OK : B_NO_MEMORY;
}
extern "C" int renku_bus_dma_forget(const void* device)
{
	if (device == NULL) return B_BAD_VALUE;
	status_t s = Lock();
	if (s != B_OK) return s;
	Binding* binding = Find(device);
	s = binding == NULL ? B_BAD_VALUE : binding->live != 0 ? B_BUSY : B_OK;
	if (s == B_OK) *binding = {};
	release_sem(sLock);
	return s;
}
struct LoadResult { int error; unsigned calls; uint64_t dma; size_t size; };
static void Loaded(void* cookie, bus_dma_segment_t* segments, int count, int error)
{
	auto& result = *(LoadResult*)cookie;
	result.calls++;
	result.error = error;
	if (error == 0 && count == 1) {
		result.dma = segments[0].ds_addr;
		result.size = segments[0].ds_len;
	} else if (error == 0) result.error = B_BAD_DATA;
}
extern "C" void* renku_bus_dma_alloc(const void* device, size_t size, uint64_t* dma)
{
	if (device == NULL || dma == NULL || size == 0 || size > SIZE_MAX - 4095)
		return NULL;
	const size_t rounded = (size + 4095) & ~(size_t)4095;
	size_t alignment = 4096;
	while (alignment < rounded) {
		if (alignment > SIZE_MAX / 2) return NULL;
		alignment *= 2;
	}
	if (Lock() != B_OK) return NULL;
	Binding* binding = Find(device);
	if (binding == NULL || binding->live == UINT_MAX) {
		release_sem(sLock);
		return NULL;
	}
	const uint64_t mask = binding->mask;
	binding->live++; // Pin binding across sleeping native allocator callbacks.
	release_sem(sLock);
	auto* allocation = (Allocation*)calloc(1, sizeof(Allocation));
	int error = allocation == NULL ? B_NO_MEMORY : 0;
	if (error == 0) {
		allocation->device = device;
		allocation->alias_area = -1;
		allocation->owned = true;
		allocation->ready = true;
		allocation->size = size;
		error = bus_dma_tag_create(NULL, alignment, 0, mask, UINT64_MAX,
			NULL, NULL, rounded, 1, rounded, 0, NULL, NULL, &allocation->tag);
	}
	if (error == 0)
		// Allocate memory separately: check dmamap creation explicitly instead
		// of relying on bus_dmamem_alloc's unchecked internal map allocation.
		error = bus_dmamem_alloc(allocation->tag, &allocation->backing,
			BUS_DMA_WAITOK | BUS_DMA_COHERENT | BUS_DMA_ZERO, NULL);
	if (error != 0 && allocation != NULL) allocation->backing = NULL;
	if (error == 0) {
		allocation->cpu = allocation->backing;
		if (((uintptr_t)allocation->cpu & (alignment - 1)) != 0) {
			// Linux coherent allocations align both CPU and DMA addresses to
			// the containing page order. Native bus_dma aligns physical backing;
			// a kernel block-address clone supplies the CPU alignment as needed.
			allocation->alias_area = clone_area("i915 coherent DMA alignment",
				&allocation->cpu, B_ANY_KERNEL_BLOCK_ADDRESS,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, area_for(allocation->backing));
			if (allocation->alias_area < B_OK) error = allocation->alias_area;
			else if (((uintptr_t)allocation->cpu & (alignment - 1)) != 0)
				error = B_BAD_VALUE;
			else
				// Populate the RAM alias now, while GFP_KERNEL permits faults.
				// Returning a lazy alias would be unsafe for later IRQ users.
				memset(allocation->cpu, 0, rounded);
		}
	}
	if (error == 0)
		error = bus_dmamap_create(allocation->tag, BUS_DMA_WAITOK, &allocation->map);
	LoadResult loaded = {};
	if (error == 0)
		error = bus_dmamap_load(allocation->tag, allocation->map, allocation->backing,
			rounded, Loaded, &loaded, BUS_DMA_WAITOK);
	if (error == 0 && (loaded.calls != 1 || loaded.error != 0
		|| loaded.size != rounded || (loaded.dma & (alignment - 1)) != 0
		|| loaded.dma > mask || rounded - 1 > mask - loaded.dma))
		error = B_BAD_DATA;
	if (error == 0) {
		// Renku's x86 direct bus_dma contract is cache coherent. i915 still
		// owns its GPU cache/domain transitions and publication barriers.
		allocation->dma = loaded.dma;
		memory_write_barrier();
		if (Lock() != B_OK) panic("i915: lost DMA allocation binding lock");
		allocation->next = sAllocations;
		sAllocations = allocation;
		release_sem(sLock);
		*dma = allocation->dma;
		return allocation->cpu;
	}
	if (allocation != NULL) {
		if (allocation->map != NULL) {
			bus_dmamap_unload(allocation->tag, allocation->map);
			bus_dmamap_destroy(allocation->tag, allocation->map);
		}
		if (allocation->alias_area >= B_OK) {
			if (delete_area(allocation->alias_area) != B_OK)
				panic("i915: failed DMA alignment alias cleanup");
		}
		if (allocation->backing != NULL)
			bus_dmamem_free(allocation->tag, allocation->backing, NULL);
		if (allocation->tag != NULL) bus_dma_tag_destroy(allocation->tag);
		free(allocation);
	}
	if (Lock() != B_OK) panic("i915: lost failed DMA allocation lock");
	Find(device)->live--;
	release_sem(sLock);
	return NULL;
}
extern "C" int renku_bus_dma_free(const void* device, size_t size, void* cpu, uint64_t dma)
{
	status_t s = Lock();
	if (s != B_OK) return s;
	Allocation** link = &sAllocations;
	while (*link != NULL && (!(*link)->owned || (*link)->cpu != cpu)) link = &(*link)->next;
	Allocation* allocation = *link;
	if (allocation == NULL || allocation->device != device || allocation->size != size
		|| allocation->dma != dma) {
		release_sem(sLock);
		return B_BAD_VALUE;
	}
	*link = allocation->next;
	release_sem(sLock);
	memory_read_barrier();
	bus_dmamap_unload(allocation->tag, allocation->map);
	if (allocation->alias_area >= B_OK && delete_area(allocation->alias_area) != B_OK)
		panic("i915: failed to retire coherent DMA CPU alias");
	bus_dmamem_free(allocation->tag, allocation->backing, allocation->map);
	s = bus_dma_tag_destroy(allocation->tag);
	if (s != 0) panic("i915: DMA tag retained after freeing its only map");
	free(allocation);
	if (Lock() != B_OK) panic("i915: lost DMA free binding lock");
	Find(device)->live--;
	release_sem(sLock);
	return B_OK;
}

extern "C" int renku_bus_dma_map(const void* device, const void* key, void* cpu,
 size_t size, int direction, int skip_sync, uint64_t* dma)
{
 if (device == NULL || key == NULL || cpu == NULL || size == 0 || dma == NULL
  || direction < 0 || direction > 2) return B_BAD_VALUE;
 auto* mapping = (Allocation*)calloc(1, sizeof(Allocation));
 if (mapping == NULL) return B_NO_MEMORY;
 status_t status = Lock();
 if (status != B_OK) { free(mapping); return status; }
 Binding* binding = Find(device);
 bool duplicate = false;
 for (Allocation* old = sAllocations; old != NULL; old = old->next)
  if (!old->owned && old->device == device && old->key == key) duplicate = true;
 if (binding == NULL || binding->live == UINT_MAX || duplicate) {
  release_sem(sLock); free(mapping); return B_BUSY;
 }
 const uint64_t mask = binding->mask;
 binding->live++;
 mapping->device = device; mapping->key = key; mapping->cpu = cpu;
 mapping->size = size; mapping->direction = direction;
 mapping->next = sAllocations; sAllocations = mapping;
 release_sem(sLock);
 // A Linux SG entry describes a physically contiguous span of pinned pages.
 // Restrict this first backend to native direct maps. In particular do not let
 // a hidden bounce buffer defeat i915's DMA_ATTR_SKIP_CPU_SYNC contract.
 physical_entry physical = {};
 uint32 count = 1;
 status = get_memory_map_etc(B_SYSTEM_TEAM, cpu, size, &physical, &count);
 if (status == B_OK && (count != 1 || physical.size < size
  || physical.address > mask || size - 1 > mask - physical.address)) status = B_BAD_VALUE;
 if (status == B_OK)
  status = bus_dma_tag_create(NULL, 1, 0, mask, UINT64_MAX, NULL, NULL,
   size, 1, size, 0, NULL, NULL, &mapping->tag);
 if (status == B_OK)
  status = bus_dmamap_create(mapping->tag, BUS_DMA_WAITOK, &mapping->map);
 LoadResult loaded = {};
 if (status == B_OK)
  status = bus_dmamap_load(mapping->tag, mapping->map, cpu, size,
   Loaded, &loaded, BUS_DMA_WAITOK);
 if (status == B_OK && (loaded.calls != 1 || loaded.error != 0
  || loaded.size != size || loaded.dma > mask || size - 1 > mask - loaded.dma))
  status = B_BAD_DATA;
 if (status == B_OK && loaded.dma != physical.address) status = B_NOT_SUPPORTED;
 if (status == B_OK && !skip_sync) memory_write_barrier();
 if (Lock() != B_OK) panic("i915: lost streaming DMA binding lock");
 if (status == B_OK) {
  mapping->dma = loaded.dma; mapping->ready = true;
  *dma = mapping->dma;
  release_sem(sLock); return B_OK;
 }
 Allocation** link = &sAllocations;
 while (*link != mapping) link = &(*link)->next;
 *link = mapping->next;
 release_sem(sLock);
 if (mapping->map != NULL) {
  bus_dmamap_unload(mapping->tag, mapping->map);
  bus_dmamap_destroy(mapping->tag, mapping->map);
 }
 if (mapping->tag != NULL) bus_dma_tag_destroy(mapping->tag);
 free(mapping);
 if (Lock() != B_OK) panic("i915: lost failed streaming DMA lock");
 Find(device)->live--;
 release_sem(sLock); return status;
}
extern "C" int renku_bus_dma_unmap(const void* device, const void* key,
 size_t size, int direction, int skip_sync)
{
 status_t status = Lock();
 if (status != B_OK) return status;
 Allocation** link = &sAllocations;
 while (*link != NULL && ((*link)->owned || (*link)->device != device || (*link)->key != key))
  link = &(*link)->next;
 Allocation* mapping = *link;
 if (mapping == NULL || !mapping->ready || mapping->size != size || mapping->direction != direction) {
  release_sem(sLock); return B_BAD_VALUE;
 }
 *link = mapping->next;
 release_sem(sLock);
 if (!skip_sync) memory_read_barrier();
 bus_dmamap_unload(mapping->tag, mapping->map);
 status = bus_dmamap_destroy(mapping->tag, mapping->map);
 if (status != 0) panic("i915: streaming DMA map teardown failed");
 status = bus_dma_tag_destroy(mapping->tag);
 if (status != 0) panic("i915: streaming DMA tag teardown failed");
 free(mapping);
 if (Lock() != B_OK) panic("i915: lost streaming DMA unmap lock");
 Find(device)->live--;
 release_sem(sLock); return B_OK;
}
