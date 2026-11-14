/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_SCHED_MM_H
#define RENKU_LINUX_SCHED_MM_H
#include <linux/gfp.h>
#include <linux/sched.h>
/* Process address spaces remain opaque. A native address-space object reference
 * does not itself keep its mappings alive, so mmget/mmgrab cannot be replaced
 * with a guessed reference count or a successful no-op. */
struct mm_struct;
void mmgrab(struct mm_struct *);
void mmdrop(struct mm_struct *);
void mmget(struct mm_struct *);
bool mmget_not_zero(struct mm_struct *);
void mmput(struct mm_struct *);
struct mm_struct *get_task_mm(struct task_struct *);
/* Allocation annotation for this non-lockdep adapter: use the existing actual
 * task/IRQ sleep-context validator when the requested policy may block. */
static inline void might_alloc(gfp_t flags)
{
	if (gfpflags_allow_blocking(flags)) might_sleep();
}
#endif
