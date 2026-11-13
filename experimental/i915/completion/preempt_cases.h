/* SPDX-License-Identifier: MIT */
#ifndef RENKU_PREEMPT_CASES_H
#define RENKU_PREEMPT_CASES_H
#include <linux/preempt.h>
#include <linux/irqflags.h>
extern bool interrupts_enabled(void);
extern int32 smp_get_current_cpu(void);
static void PreemptCases(struct completion_report *r)
{
	spinlock_t spin;
	rwlock_t rw;
	spin_lock_init(&spin); rwlock_init(&rw);
	CHECK(r, preempt_count() == 0 && preemptible() && !in_atomic());
	preempt_disable();
	int32 cpu = smp_get_current_cpu();
	CHECK(r, preempt_count() == 1 && in_atomic() && !preemptible() && !interrupts_enabled());
	preempt_disable();
	CHECK(r, preempt_count() == 2 && smp_get_current_cpu() == cpu);
	preempt_enable();
	CHECK(r, preempt_count() == 1 && !interrupts_enabled());
	/* _irq requests enable, but an outer preemption scope must keep the CPU
	 * physically excluded until its own exit. */
	raw_spin_lock_irq(&spin.rlock);
	CHECK(r, preempt_count() == 2);
	raw_spin_unlock_irq(&spin.rlock);
	CHECK(r, preempt_count() == 1 && !interrupts_enabled());
	unsigned long flags;
	local_irq_save(flags);
	CHECK(r, !irqs_disabled_flags(flags));
	local_irq_restore(flags);
	CHECK(r, !interrupts_enabled());
	CHECK(r, renku_linux_preempt_backend_uninit() == B_BUSY);
	preempt_enable();
	CHECK(r, preempt_count() == 0 && interrupts_enabled());
	/* Explicit IRQ state changes survive an enclosing preemption scope. */
	preempt_disable(); local_irq_disable(); preempt_enable();
	CHECK(r, preempt_count() == 0 && !interrupts_enabled() && in_atomic());
	local_irq_enable();
	CHECK(r, preemptible());
	local_irq_disable(); preempt_disable(); local_irq_enable();
	CHECK(r, preempt_count() == 1 && !interrupts_enabled());
	preempt_enable();
	CHECK(r, interrupts_enabled());
	/* Overlapping lock/preempt lifetimes are not constrained to a single
	 * LIFO IRQ-save field. Releasing either one must retain the other. */
	spin_lock(&spin); preempt_disable(); spin_unlock(&spin);
	CHECK(r, preempt_count() == 1 && !interrupts_enabled());
	preempt_enable();
	CHECK(r, preempt_count() == 0 && interrupts_enabled());
	preempt_disable(); read_lock(&rw); preempt_enable();
	CHECK(r, preempt_count() == 1 && !interrupts_enabled());
	read_unlock(&rw);
	CHECK(r, preempt_count() == 0 && interrupts_enabled());
	local_irq_save(flags);
	unsigned long nested;
	raw_spin_lock_irqsave(&spin.rlock, nested); preempt_disable();
	raw_spin_unlock_irqrestore(&spin.rlock, nested); preempt_enable();
	CHECK(r, !interrupts_enabled() && preempt_count() == 0);
	local_irq_restore(flags);
	CHECK(r, interrupts_enabled() && preemptible());
}
#endif
