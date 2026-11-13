/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_SCHED_H
#define RENKU_LINUX_SCHED_H
#include <kpi_base.h>
#include <linux/ktime.h>
#ifndef RENKU_NATIVE
#include <pthread.h>
#include <semaphore.h>
#endif
/* Stable identity for a registered kernel-entry scope, not a Linux scheduler. */
#define TASK_COMM_LEN 16
struct task_struct {
	char comm[TASK_COMM_LEN];
#ifdef RENKU_NATIVE
	thread_id native_id;
#else
	pthread_t native_id;
#endif
	struct task_struct *registry_next;
	/* Event storage is initialized lazily by the native wait adapter, before
	 * publishing this task on a wait queue. The registering caller owns it. */
#ifdef RENKU_NATIVE
	sem_id wait_event;
#else
	sem_t wait_event;
#endif
	bool wait_initialized;
	unsigned int wait_state, interrupted_state;
	void (*release_wait)(struct task_struct *);
};
bool renku_linux_task_register(struct task_struct *);
void renku_linux_task_unregister(struct task_struct *);
bool renku_linux_tasks_empty(void);
struct task_struct *renku_linux_current(void);
#define current renku_linux_current()
#define MAX_SCHEDULE_TIMEOUT LONG_MAX
#define TASK_RUNNING 0
#define TASK_INTERRUPTIBLE 1
#define TASK_UNINTERRUPTIBLE 2
#define TASK_WAKEKILL 0x100
#define TASK_KILLABLE (TASK_WAKEKILL | TASK_UNINTERRUPTIBLE)
#define TASK_NORMAL (TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)
void renku_linux_might_sleep(void);
void might_sleep(void);
void renku_linux_set_current_state(unsigned int);
int renku_linux_signal_pending_state(unsigned int, struct task_struct *);
int try_to_wake_up(struct task_struct *, unsigned int, int);
int wake_up_state(struct task_struct *, unsigned int);
long schedule_timeout(long);
long io_schedule_timeout(long);
#define set_current_state renku_linux_set_current_state
#define __set_current_state renku_linux_set_current_state
#define signal_pending_state renku_linux_signal_pending_state
#include <asm/processor.h>
#endif
