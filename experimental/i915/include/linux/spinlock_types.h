/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_SPINLOCK_TYPES_H
#define RENKU_LINUX_SPINLOCK_TYPES_H
#include <kpi_base.h>
#ifdef RENKU_NATIVE
typedef struct { spinlock native; cpu_status saved_interrupts; } spinlock_t;
typedef struct { rw_spinlock native; } rwlock_t;
#else
/* Host tests use actual pthread locks, never a successful no-op. */
#include <pthread.h>
typedef struct { pthread_mutex_t native; } spinlock_t;
typedef struct { pthread_rwlock_t native; } rwlock_t;
#endif
#endif
