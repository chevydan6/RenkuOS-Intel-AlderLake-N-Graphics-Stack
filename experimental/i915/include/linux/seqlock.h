/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_SEQLOCK_H
#define RENKU_LINUX_SEQLOCK_H
#include <kpi_base.h>
#include <asm/barrier.h>
#include <linux/irqflags.h>
/* Complete original non-RT/non-debug algorithms. Writers must satisfy the
 * original serialization/non-preemptibility contract. Native spinlocks mask
 * IRQs; readers may not interrupt their own writer. BH operations stay
 * unresolved until native softirq entry/exit is integrated. */
#include "../../upstream/linux/seqlock.h"
#endif
