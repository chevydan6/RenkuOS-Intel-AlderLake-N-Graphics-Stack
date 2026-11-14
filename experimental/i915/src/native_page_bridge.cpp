// SPDX-License-Identifier: MIT
// Keep actual native VM/C++ layouts out of the Linux compatibility namespace.
#include <KernelExport.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/VMCache.h>
#include <vm_defs.h>
#include <stdlib.h>
#include <string.h>
#include <native_page_bridge.h>
extern "C" bool interrupts_enabled(void);
struct renku_page_event { sem_id semaphore; };
struct renku_page_worker { thread_id thread; };
extern "C" int32_t renku_vm_page_run_create(size_t size, void **cpu, uint64_t *physical)
{
	if (!cpu || !physical || size < B_PAGE_SIZE || (size & (size - 1)) || !interrupts_enabled())
		return B_BAD_VALUE;
	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
	virtualRestrictions.alignment = size;
	physical_address_restrictions physicalRestrictions = {};
	physicalRestrictions.alignment = size;
	void *base = NULL;
	area_id area = create_area_etc(B_SYSTEM_TEAM, "i915 Linux owned pages", size,
		B_CONTIGUOUS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0, 0,
		&virtualRestrictions, &physicalRestrictions, &base);
	if (area < B_OK) return area;
	physical_entry entry;
	uint32 count = 1;
	status_t status = get_memory_map_etc(B_SYSTEM_TEAM, base, size, &entry, &count);
	if (status != B_OK || count != 1 || entry.size < size
		|| ((uintptr_t)base & (size - 1)) || (entry.address & (size - 1))
		|| entry.address > UINT64_MAX - (size - 1)) {
		if (delete_area(area) != B_OK) panic("i915 page allocation rollback failed");
		return status != B_OK ? status : B_BAD_DATA;
	}
	*cpu = base; *physical = entry.address;
	return area;
}
extern "C" int32_t renku_vm_page_run_delete(int32_t area) { return delete_area(area); }
extern "C" int32_t renku_vm_page_store_create(size_t size, void **cpu)
{
	return renku_vm_page_store_create_policy(size, false, cpu);
}
extern "C" int32_t renku_vm_page_store_create_policy(size_t size, bool noreserve, void **cpu)
{
	if (!cpu || !size || (size & (B_PAGE_SIZE - 1)) || !interrupts_enabled())
		return B_BAD_VALUE;
	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
	physical_address_restrictions physicalRestrictions = {};
	return create_area_etc(B_SYSTEM_TEAM, "i915 Linux pageable store", size,
		B_NO_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA
			| (noreserve ? B_OVERCOMMITTING_AREA : 0),
		noreserve ? CREATE_AREA_DONT_COMMIT_MEMORY : 0, 0,
		&virtualRestrictions, &physicalRestrictions, cpu);
}
extern "C" int32_t renku_vm_page_store_pin(void *cpu, uint64_t *physical)
{
	if (!cpu || !physical || ((uintptr_t)cpu & (B_PAGE_SIZE - 1)) || !interrupts_enabled())
		return B_BAD_VALUE;
	status_t status = lock_memory_etc(B_SYSTEM_TEAM, cpu, B_PAGE_SIZE, B_READ_DEVICE);
	if (status != B_OK) return status;
	physical_entry entry;
	uint32 count = 1;
	status = get_memory_map_etc(B_SYSTEM_TEAM, cpu, B_PAGE_SIZE, &entry, &count);
	if (status != B_OK || count != 1 || entry.size < B_PAGE_SIZE
		|| (entry.address & (B_PAGE_SIZE - 1)) || entry.address > UINT64_MAX - (B_PAGE_SIZE - 1)) {
		if (unlock_memory_etc(B_SYSTEM_TEAM, cpu, B_PAGE_SIZE, B_READ_DEVICE) != B_OK)
			panic("i915 store pin rollback failed");
		return status != B_OK ? status : B_BAD_DATA;
	}
	*physical = entry.address;
	return B_OK;
}
extern "C" int32_t renku_vm_page_store_unpin(void *cpu)
{
	return unlock_memory_etc(B_SYSTEM_TEAM, cpu, B_PAGE_SIZE, B_READ_DEVICE);
}
extern "C" int32_t renku_vm_page_state_update(uint64_t physical, uint32_t requested, uint32_t *previous)
{
	if (!previous || (physical & (B_PAGE_SIZE - 1))
		|| (requested & ~(RENKU_VM_PAGE_DIRTY | RENKU_VM_PAGE_ACCESSED))
		|| !interrupts_enabled()) return B_BAD_VALUE;
	/* The caller's pin protects physical identity through lookup and locking.
	 * A wired-count check cannot establish that ownership for an arbitrary PFN. */
	vm_page *page = vm_lookup_page(physical / B_PAGE_SIZE);
	if (!page) return B_BAD_VALUE;
	VMCache *cache = vm_cache_acquire_locked_page_cache(page, false);
	if (!cache) return B_BAD_VALUE;
	status_t status = B_OK;
	if (!page->WiredCount()) status = B_BAD_VALUE;
	else {
		uint32_t state = (page->modified ? RENKU_VM_PAGE_DIRTY : 0)
			| (page->accessed ? RENKU_VM_PAGE_ACCESSED : 0);
		if (requested & RENKU_VM_PAGE_DIRTY) page->modified = true;
		if (requested & RENKU_VM_PAGE_ACCESSED) page->accessed = true;
		*previous = state;
	}
	cache->ReleaseRefAndUnlock();
	return status;
}
extern "C" struct renku_page_event *renku_vm_page_event_create(void)
{
	auto *event = (renku_page_event *)malloc(sizeof(renku_page_event));
	if (!event) return NULL;
	event->semaphore = create_sem(0, "i915 Linux page release");
	if (event->semaphore < B_OK) { free(event); return NULL; }
	return event;
}
extern "C" void renku_vm_page_event_delete(struct renku_page_event *event)
{
	if (delete_sem(event->semaphore) != B_OK) panic("i915 page event deletion failed");
	free(event);
}
extern "C" void renku_vm_page_event_signal(struct renku_page_event *event)
{
	if (release_sem_etc(event->semaphore, 1, B_DO_NOT_RESCHEDULE) != B_OK)
		panic("i915 page release signal failed");
}
extern "C" void renku_vm_page_event_wait(struct renku_page_event *event)
{
	status_t status;
	do { status = acquire_sem(event->semaphore); } while (status == B_INTERRUPTED);
	if (status != B_OK) panic("i915 page release wait failed");
}
extern "C" struct renku_page_worker *renku_vm_page_worker_start(int32_t (*entry)(void *), void *cookie)
{
	auto *worker = (renku_page_worker *)malloc(sizeof(renku_page_worker));
	if (!worker) return NULL;
	worker->thread = spawn_kernel_thread(entry, "i915 Linux page release", B_NORMAL_PRIORITY, cookie);
	if (worker->thread < B_OK) { free(worker); return NULL; }
	if (resume_thread(worker->thread) != B_OK) panic("i915 page worker resume failed");
	return worker;
}
extern "C" void renku_vm_page_worker_join(struct renku_page_worker *worker)
{
	status_t returned;
	status_t status = wait_for_thread(worker->thread, &returned);
	if (status == B_BAD_THREAD_ID) {
		thread_info info;
		if (get_thread_info(worker->thread, &info) != B_BAD_THREAD_ID)
			panic("i915 page worker join identity changed");
	} else if (status != B_OK || returned != B_OK) panic("i915 page worker join failed");
	free(worker);
}
extern "C" int64_t renku_vm_page_now(void) { return system_time(); }
extern "C" void renku_vm_page_pause(void) { snooze(1000); }
extern "C" void renku_vm_pagefault_forbid(void) { forbid_page_faults(); }
extern "C" void renku_vm_pagefault_permit(void) { permit_page_faults(); }
