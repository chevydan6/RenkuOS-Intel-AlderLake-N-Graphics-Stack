/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_SPINLOCK_TYPES_H
#define RENKU_LINUX_SPINLOCK_TYPES_H
#include <kpi_base.h>
#include <linux/lockdep_types.h>
#ifdef CONFIG_PREEMPT_RT
#error Raw/regular spinlock equivalence requires the non-RT configuration
#endif
#ifdef RENKU_NATIVE
typedef struct { spinlock native; cpu_status saved_interrupts; } spinlock_t;
typedef struct { rw_spinlock native; } rwlock_t;
#else
/* Host tests use actual pthread locks, never a successful no-op. */
#include <pthread.h>
typedef struct { pthread_mutex_t native; unsigned long saved_interrupts; } spinlock_t;
typedef struct { pthread_rwlock_t native; } rwlock_t;
#endif
typedef spinlock_t raw_spinlock_t;
#endif
