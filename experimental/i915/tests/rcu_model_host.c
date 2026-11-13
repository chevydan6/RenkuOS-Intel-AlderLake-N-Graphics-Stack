/* SPDX-License-Identifier: MIT */
/* Compile separately with host headers: never reinterpret a native lock as a
 * pthread object. The model delegates native API operations to this lock. */
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
static pthread_rwlock_t readers;
static pthread_t threads[2];
static void *expected_lock;
static int writer_attempt;
void model_init(void)
{
	pthread_rwlockattr_t attr;
	if (pthread_rwlockattr_init(&attr)
		|| pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP)
		|| pthread_rwlock_init(&readers, &attr)) abort();
	pthread_rwlockattr_destroy(&attr);
}
static void Check(void *lock)
{ if (!expected_lock) expected_lock = lock; if (expected_lock != lock) abort(); }
void model_read_lock(void *lock) { Check(lock); if (pthread_rwlock_rdlock(&readers)) abort(); }
void model_read_unlock(void *lock) { Check(lock); if (pthread_rwlock_unlock(&readers)) abort(); }
void model_write_lock(void *lock)
{ Check(lock); __atomic_store_n(&writer_attempt, 1, __ATOMIC_RELEASE); if (pthread_rwlock_wrlock(&readers)) abort(); }
void model_write_unlock(void *lock) { Check(lock); if (pthread_rwlock_unlock(&readers)) abort(); }
int model_writer_attempt(void) { return __atomic_load_n(&writer_attempt, __ATOMIC_ACQUIRE); }
void model_start(unsigned i, void *(*function)(void *), void *cookie)
{ if (i >= 2 || pthread_create(&threads[i], NULL, function, cookie)) abort(); }
void model_join(unsigned i) { if (pthread_join(threads[i], NULL)) abort(); }
int64_t model_now_us(void)
{ struct timespec t; if (clock_gettime(CLOCK_MONOTONIC, &t)) abort(); return (int64_t)t.tv_sec * 1000000 + t.tv_nsec / 1000; }
void model_pause(void) { sched_yield(); }
void model_finish(void) { if (pthread_rwlock_destroy(&readers)) abort(); }
