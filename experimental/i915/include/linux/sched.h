/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_SCHED_H
#define RENKU_LINUX_SCHED_H
#include <kpi_base.h>
#include <linux/ktime.h>
#ifndef RENKU_NATIVE
#include <pthread.h>
#endif
/* Stable identity for a registered kernel-entry scope, not a Linux scheduler. */
struct task_struct {
#ifdef RENKU_NATIVE
	thread_id native_id;
#else
	pthread_t native_id;
#endif
	struct task_struct *registry_next;
};
bool renku_linux_task_register(struct task_struct *);
void renku_linux_task_unregister(struct task_struct *);
struct task_struct *renku_linux_current(void);
#define current renku_linux_current()
#define MAX_SCHEDULE_TIMEOUT LONG_MAX
static inline void cpu_relax(void)
{
	__asm__ __volatile__("pause" ::: "memory");
}
#endif
