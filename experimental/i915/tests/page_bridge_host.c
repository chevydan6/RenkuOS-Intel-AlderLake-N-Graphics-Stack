/* SPDX-License-Identifier: MIT */
/* Explicit OS model using real host RAM, pthread workers and POSIX semaphores.
 * Synthetic physical IDs are test metadata, never native/DMA evidence. */
#include <native_page_bridge.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
struct renku_page_event { sem_t sem; };
struct renku_page_worker { pthread_t thread; int32_t (*entry)(void *); void *cookie; int32_t result; };
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static struct { void *base; size_t size; bool store; unsigned *pins, *generation; } runs[32];
static unsigned fail_create, fail_event, fail_worker, hold_delete;
static unsigned fail_pin, fail_store_delete, hold_pin, pin_entered, hold_unpin;
static _Thread_local unsigned faults;
void renku_test_page_fail_create(void) { __atomic_store_n(&fail_create, 1, __ATOMIC_RELEASE); }
void renku_test_page_fail_event(void) { __atomic_store_n(&fail_event, 1, __ATOMIC_RELEASE); }
void renku_test_page_fail_worker(void) { __atomic_store_n(&fail_worker, 1, __ATOMIC_RELEASE); }
void renku_test_page_hold_delete(unsigned hold) { __atomic_store_n(&hold_delete, hold, __ATOMIC_RELEASE); }
void renku_test_store_fail_pin(void) { __atomic_store_n(&fail_pin, 1, __ATOMIC_RELEASE); }
void renku_test_store_fail_delete(void) { __atomic_store_n(&fail_store_delete, 1, __ATOMIC_RELEASE); }
void renku_test_store_hold_pin(unsigned hold) { __atomic_store_n(&hold_pin, hold, __ATOMIC_RELEASE); }
void renku_test_store_hold_unpin(unsigned hold) { __atomic_store_n(&hold_unpin, hold, __ATOMIC_RELEASE); }
unsigned renku_test_store_pin_entered(void) { return __atomic_load_n(&pin_entered, __ATOMIC_ACQUIRE); }
unsigned renku_test_store_pinned_count(void)
{
	assert(!pthread_mutex_lock(&lock)); unsigned count = 0;
	for (unsigned i = 0; i < 32; i++) if (runs[i].store)
		for (size_t j = 0; j < runs[i].size / 4096; j++) count += runs[i].pins[j];
	assert(!pthread_mutex_unlock(&lock)); return count;
}
unsigned renku_test_page_fault_depth(void) { return faults; }
unsigned renku_test_page_live_count(void)
{
	assert(!pthread_mutex_lock(&lock)); unsigned count = 0;
	for (unsigned i = 0; i < 32; i++) count += runs[i].base != NULL;
	assert(!pthread_mutex_unlock(&lock)); return count;
}
int32_t renku_vm_page_run_create(size_t size, void **cpu, uint64_t *physical)
{
	if (__atomic_exchange_n(&fail_create, 0, __ATOMIC_ACQ_REL) || size > 0x10000000) return -1;
	void *base = NULL;
	if (posix_memalign(&base, size, size)) return -1;
	assert(!pthread_mutex_lock(&lock));
	for (unsigned i = 0; i < 32; i++) if (!runs[i].base) {
		runs[i].base = base; runs[i].size = size;
		*cpu = base; *physical = 0x100000000ULL + (uint64_t)i * 0x10000000;
		assert(!pthread_mutex_unlock(&lock)); return (int32_t)i;
	}
	assert(!pthread_mutex_unlock(&lock)); free(base); return -1;
}
int32_t renku_vm_page_run_delete(int32_t id)
{
	while (__atomic_load_n(&hold_delete, __ATOMIC_ACQUIRE)) renku_vm_page_pause();
	assert(id >= 0 && id < 32); assert(!pthread_mutex_lock(&lock));
	assert(runs[id].base);
	if (runs[id].store) {
		if (__atomic_exchange_n(&fail_store_delete, 0, __ATOMIC_ACQ_REL)) {
			assert(!pthread_mutex_unlock(&lock)); return -1;
		}
		for (size_t i = 0; i < runs[id].size / 4096; i++) assert(!runs[id].pins[i]);
		free(runs[id].pins); free(runs[id].generation);
	}
	free(runs[id].base); memset(&runs[id], 0, sizeof(runs[id]));
	assert(!pthread_mutex_unlock(&lock)); return 0;
}
int32_t renku_vm_page_store_create(size_t size, void **cpu)
{
	if (__atomic_exchange_n(&fail_create, 0, __ATOMIC_ACQ_REL) || size > 0x10000000) return -1;
	assert(size && !(size & 4095));
	void *base = NULL;
	if (posix_memalign(&base, 4096, size)) return -1;
	memset(base, 0, size); /* Real RAM model; native zero faults/reclaim are not modeled. */
	unsigned *pins = calloc(size / 4096, sizeof(*pins));
	unsigned *generation = calloc(size / 4096, sizeof(*generation));
	assert(pins && generation);
	assert(!pthread_mutex_lock(&lock));
	for (unsigned i = 0; i < 32; i++) if (!runs[i].base) {
		runs[i].base = base; runs[i].size = size; runs[i].store = true;
		runs[i].pins = pins; runs[i].generation = generation; *cpu = base;
		assert(!pthread_mutex_unlock(&lock)); return (int32_t)i;
	}
	assert(!pthread_mutex_unlock(&lock)); free(pins); free(generation); free(base); return -1;
}
static unsigned StoreAddress(void *cpu, size_t *index)
{
	for (unsigned i = 0; i < 32; i++) if (runs[i].store) {
		uintptr_t offset = (uintptr_t)cpu - (uintptr_t)runs[i].base;
		if (offset < runs[i].size && !(offset & 4095)) { *index = offset / 4096; return i; }
	}
	abort();
}
int32_t renku_vm_page_store_pin(void *cpu, uint64_t *physical)
{
	__atomic_fetch_add(&pin_entered, 1, __ATOMIC_RELEASE);
	while (__atomic_load_n(&hold_pin, __ATOMIC_ACQUIRE)) renku_vm_page_pause();
	if (__atomic_exchange_n(&fail_pin, 0, __ATOMIC_ACQ_REL)) return -1;
	assert(!pthread_mutex_lock(&lock));
	size_t index; unsigned i = StoreAddress(cpu, &index);
	if (!runs[i].pins[index]++) runs[i].generation[index]++;
	/* Deliberately change synthetic physical identity after complete unpin.
	 * Overlapping pins must retain the same identity and original contents. */
	*physical = 0x10000000000ULL + (uint64_t)i * 0x40000000
		+ index * 8192 + (runs[i].generation[index] & 1) * 4096;
	assert(!pthread_mutex_unlock(&lock)); return 0;
}
int32_t renku_vm_page_store_unpin(void *cpu)
{
	while (__atomic_load_n(&hold_unpin, __ATOMIC_ACQUIRE)) renku_vm_page_pause();
	assert(!pthread_mutex_lock(&lock));
	size_t index; unsigned i = StoreAddress(cpu, &index);
	assert(runs[i].pins[index]); runs[i].pins[index]--;
	assert(!pthread_mutex_unlock(&lock)); return 0;
}
struct renku_page_event *renku_vm_page_event_create(void)
{
	if (__atomic_exchange_n(&fail_event, 0, __ATOMIC_ACQ_REL)) return NULL;
	struct renku_page_event *e = malloc(sizeof(*e)); assert(e);
	assert(!sem_init(&e->sem, 0, 0)); return e;
}
void renku_vm_page_event_delete(struct renku_page_event *e) { assert(!sem_destroy(&e->sem)); free(e); }
void renku_vm_page_event_signal(struct renku_page_event *e) { assert(!sem_post(&e->sem)); }
void renku_vm_page_event_wait(struct renku_page_event *e)
{
	int status; do { status = sem_wait(&e->sem); } while (status && errno == EINTR);
	assert(!status);
}
static void *Worker(void *cookie)
{
	struct renku_page_worker *w = cookie; w->result = w->entry(w->cookie); return NULL;
}
struct renku_page_worker *renku_vm_page_worker_start(int32_t (*entry)(void *), void *cookie)
{
	if (__atomic_exchange_n(&fail_worker, 0, __ATOMIC_ACQ_REL)) return NULL;
	struct renku_page_worker *w = malloc(sizeof(*w)); assert(w);
	w->entry = entry; w->cookie = cookie;
	assert(!pthread_create(&w->thread, NULL, Worker, w)); return w;
}
void renku_vm_page_worker_join(struct renku_page_worker *w)
{ assert(!pthread_join(w->thread, NULL) && w->result == 0); free(w); }
int64_t renku_vm_page_now(void)
{ struct timespec now; assert(!clock_gettime(CLOCK_MONOTONIC, &now)); return (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000; }
void renku_vm_page_pause(void) { struct timespec pause = {.tv_nsec = 1000000}; nanosleep(&pause, NULL); }
void renku_vm_pagefault_forbid(void) { faults++; }
void renku_vm_pagefault_permit(void) { assert(faults); faults--; }
