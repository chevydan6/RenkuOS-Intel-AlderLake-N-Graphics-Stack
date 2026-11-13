/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_MUTEX_H
#define RENKU_LINUX_MUTEX_H
#include <kpi_base.h>
#include <linux/atomic.h>
#include <linux/sched.h>
#ifndef RENKU_NATIVE
#include <pthread.h>
#endif
struct mutex {
#ifdef RENKU_NATIVE
	sem_id semaphore;
#else
	pthread_mutex_t native;
#endif
	int held;
};
/* Per-class identity tokens. Lockdep instrumentation itself is excluded. */
#ifdef CONFIG_DEBUG_LOCK_ALLOC
#error Linux lockdep instrumentation is not implemented
#endif
struct lock_class_key { unsigned char identity; };
void renku_linux_mutex_init_named(struct mutex *, const char *, struct lock_class_key *);
void renku_linux_mutex_init(struct mutex *);
void renku_linux_mutex_destroy(struct mutex *);
void renku_linux_mutex_lock(struct mutex *);
void renku_linux_mutex_unlock(struct mutex *);
#define mutex_init renku_linux_mutex_init
#define mutex_destroy renku_linux_mutex_destroy
#define mutex_lock renku_linux_mutex_lock
#define mutex_unlock renku_linux_mutex_unlock
#define __mutex_init renku_linux_mutex_init_named
static inline bool mutex_is_locked(const struct mutex *lock)
{
	return __atomic_load_n(&lock->held, __ATOMIC_ACQUIRE) != 0;
}
#endif
