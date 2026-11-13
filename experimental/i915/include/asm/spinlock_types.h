/* SPDX-License-Identifier: MIT */
#ifndef RENKU_ARCH_SPINLOCK_TYPES_H
#define RENKU_ARCH_SPINLOCK_TYPES_H
#include <kpi_base.h>
#if defined(CONFIG_PREEMPT_RT) || defined(CONFIG_DEBUG_SPINLOCK)
#error RT/debug-spinlock semantics are not supplied by this native adapter
#endif
#ifdef RENKU_NATIVE
typedef struct { spinlock native; } arch_spinlock_t;
typedef struct { rw_spinlock native; } arch_rwlock_t;
#define __ARCH_SPIN_LOCK_UNLOCKED { .native = B_SPINLOCK_INITIALIZER }
#define __ARCH_RW_LOCK_UNLOCKED { .native = {0} }
#else
#include <pthread.h>
/* Host tests use actual locks; IRQ state is only a CPU-test model. */
typedef struct { pthread_mutex_t native; unsigned long saved_interrupts; } arch_spinlock_t;
typedef struct { pthread_rwlock_t native; } arch_rwlock_t;
#define __ARCH_SPIN_LOCK_UNLOCKED { .native = PTHREAD_MUTEX_INITIALIZER }
#define __ARCH_RW_LOCK_UNLOCKED { .native = PTHREAD_RWLOCK_INITIALIZER }
#endif
#endif
