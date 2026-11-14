/* SPDX-License-Identifier: MIT */
#ifndef RENKU_NATIVE_PAGE_BRIDGE_H
#define RENKU_NATIVE_PAGE_BRIDGE_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
struct renku_page_event;
struct renku_page_worker;
/* Native status/area IDs stay within this bridge. No DMA address is returned. */
int32_t renku_vm_page_run_create(size_t, void **, uint64_t *);
int32_t renku_vm_page_run_delete(int32_t);
/* Pageable anonymous backing. Pin before accessing; physical identity is valid
 * only until the matching unpin. Both pin and unpin require sleepable context. */
int32_t renku_vm_page_store_create(size_t, void **);
int32_t renku_vm_page_store_create_policy(size_t, bool noreserve, void **);
int32_t renku_vm_page_store_pin(void *, uint64_t *);
int32_t renku_vm_page_store_unpin(void *);
/* Sleepable operation; caller retains a writable native pin throughout. Native
 * software flags only, not hardware PTE harvesting or Linux writeback state.
 * OR requested flags and return the previous flags under the native cache lock.
 * Zero requests query. Unknown bits/invalid inputs fail without writing output.
 * Physical identity by itself does not convey ownership. */
#define RENKU_VM_PAGE_DIRTY UINT32_C(1)
#define RENKU_VM_PAGE_ACCESSED UINT32_C(2)
int32_t renku_vm_page_state_update(uint64_t physical, uint32_t requested, uint32_t *previous);
struct renku_page_event *renku_vm_page_event_create(void);
void renku_vm_page_event_delete(struct renku_page_event *);
void renku_vm_page_event_signal(struct renku_page_event *);
void renku_vm_page_event_wait(struct renku_page_event *);
struct renku_page_worker *renku_vm_page_worker_start(int32_t (*)(void *), void *);
void renku_vm_page_worker_join(struct renku_page_worker *);
int64_t renku_vm_page_now(void);
void renku_vm_page_pause(void);
void renku_vm_pagefault_forbid(void);
void renku_vm_pagefault_permit(void);
#ifdef __cplusplus
}
#endif
#endif
