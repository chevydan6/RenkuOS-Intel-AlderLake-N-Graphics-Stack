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
struct renku_page_event { sem_t sem; };
struct renku_page_worker { pthread_t thread; int32_t (*entry)(void *); void *cookie; int32_t result; };
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static struct { void *base; size_t size; } runs[32];
static unsigned fail_create, fail_event, fail_worker, hold_delete;
static _Thread_local unsigned faults;
void renku_test_page_fail_create(void) { __atomic_store_n(&fail_create, 1, __ATOMIC_RELEASE); }
void renku_test_page_fail_event(void) { __atomic_store_n(&fail_event, 1, __ATOMIC_RELEASE); }
void renku_test_page_fail_worker(void) { __atomic_store_n(&fail_worker, 1, __ATOMIC_RELEASE); }
void renku_test_page_hold_delete(unsigned hold) { __atomic_store_n(&hold_delete, hold, __ATOMIC_RELEASE); }
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
	assert(runs[id].base); free(runs[id].base); runs[id].base = NULL;
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
