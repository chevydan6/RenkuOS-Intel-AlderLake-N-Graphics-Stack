/* SPDX-License-Identifier: MIT */
#ifndef RENKU_ASM_BARRIER_H
#define RENKU_ASM_BARRIER_H
#include <kpi_base.h>
#define smp_mb() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define smp_mb__after_atomic() __atomic_thread_fence(__ATOMIC_SEQ_CST)
/* x86-64 ordinary cacheable RAM ordering. These compiler fences pair with
 * Linux's original seqcount read/write barriers; they do not order MMIO or
 * replace i915 DMA/cache-domain transitions. */
#define smp_rmb() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define smp_wmb() __atomic_thread_fence(__ATOMIC_RELEASE)
#endif
