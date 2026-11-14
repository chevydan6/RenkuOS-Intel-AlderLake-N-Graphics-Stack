/* SPDX-License-Identifier: MIT */
#ifndef RENKU_NATIVE_PAGE_STATE_CASES_H
#define RENKU_NATIVE_PAGE_STATE_CASES_H
#include <native_page_bridge.h>
extern status_t renku_test_vm_page_state(uint64_t, bool, uint32_t *);
struct native_state_worker {
	uint64_t physical; uint32_t requested; unsigned errors;
	sem_id ready, gate;
};
static int32 NativeStateWorker(void *cookie)
{
	struct native_state_worker *w = cookie;
	if (release_sem(w->ready) != B_OK || acquire_sem(w->gate) != B_OK) Fatal();
	for (unsigned i = 0; i < 256; i++) {
		uint32_t previous = 0, actual = 0;
		if (renku_vm_page_state_update(w->physical, w->requested, &previous) != B_OK
			|| renku_test_vm_page_state(w->physical, false, &actual) != B_OK
			|| !(actual & w->requested) || (i && !(previous & w->requested))) w->errors++;
	}
	return B_OK;
}
static void NativePageStateCases(struct completion_report *r)
{
	struct renku_page_store *store = renku_linux_page_store_create(PAGE_SIZE);
	CHECK(r, store != NULL); if (!store) Fatal();
	struct folio *folio = renku_linux_page_store_read(store, 0);
	CHECK(r, folio != NULL); if (!folio) Fatal();
	uint64_t physical = page_to_phys(&folio->page);
	uint32_t previous = 0x915, actual = 0x915;
	CHECK(r, renku_test_vm_page_state(physical, true, &actual) == B_OK && actual == 0);
	CHECK(r, renku_vm_page_state_update(physical, 0, &previous) == B_OK && previous == 0);
	CHECK(r, renku_vm_page_state_update(physical, RENKU_VM_PAGE_DIRTY, &previous) == B_OK && previous == 0);
	CHECK(r, renku_test_vm_page_state(physical, false, &actual) == B_OK && actual == RENKU_VM_PAGE_DIRTY);
	CHECK(r, renku_vm_page_state_update(physical, RENKU_VM_PAGE_DIRTY, &previous) == B_OK && previous == RENKU_VM_PAGE_DIRTY);
	CHECK(r, renku_vm_page_state_update(physical, RENKU_VM_PAGE_ACCESSED, &previous) == B_OK && previous == RENKU_VM_PAGE_DIRTY);
	CHECK(r, renku_test_vm_page_state(physical, false, &actual) == B_OK && actual == 3);
	CHECK(r, renku_vm_page_state_update(physical, 3, &previous) == B_OK && previous == 3);
	CHECK(r, renku_test_vm_page_state(physical, true, &actual) == B_OK && actual == 0);
	previous = 0x915;
	CHECK(r, renku_vm_page_state_update(physical, 7, &previous) != B_OK && previous == 0x915);
	CHECK(r, renku_vm_page_state_update(physical + 1, 3, &previous) != B_OK && previous == 0x915);
	CHECK(r, renku_vm_page_state_update(physical, 3, NULL) != B_OK);
	cpu_status irq = disable_interrupts();
	status_t rejected = renku_vm_page_state_update(physical, 3, &previous);
	restore_interrupts(irq);
	CHECK(r, rejected != B_OK && previous == 0x915);
	CHECK(r, renku_test_vm_page_state(physical, false, &actual) == B_OK && actual == 0);
	sem_id ready = create_sem(0, "i915 page state ready"), gate = create_sem(0, "i915 page state gate");
	if (ready < B_OK || gate < B_OK) Fatal();
	struct native_state_worker workers[2] = {
		{physical, RENKU_VM_PAGE_DIRTY, 0, ready, gate},
		{physical, RENKU_VM_PAGE_ACCESSED, 0, ready, gate}};
	thread_id ids[2];
	for (unsigned i = 0; i < 2; i++) {
		ids[i] = spawn_kernel_thread(NativeStateWorker, "i915 native page flags", B_NORMAL_PRIORITY, &workers[i]);
		if (ids[i] < B_OK || resume_thread(ids[i]) != B_OK) Fatal();
	}
	if (acquire_sem(ready) != B_OK || acquire_sem(ready) != B_OK
		|| release_sem_etc(gate, 2, 0) != B_OK) Fatal();
	Join(ids[0]); Join(ids[1]);
	CHECK(r, delete_sem(ready) == B_OK && delete_sem(gate) == B_OK);
	CHECK(r, !workers[0].errors && !workers[1].errors);
	CHECK(r, renku_test_vm_page_state(physical, false, &actual) == B_OK && actual == 3);
	folio_put(folio);
	CHECK(r, renku_linux_pages_drain(5000) == 0);
	CHECK(r, renku_linux_page_store_destroy(store) == 0);
}
#endif
