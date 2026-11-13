/* SPDX-License-Identifier: MIT */
#ifndef RENKU_COMPLETION_PORT_H
#define RENKU_COMPLETION_PORT_H
/* Linux sched/build_utility.c normally supplies the compilation environment for
 * these original .c files. Supply native services, without editing their bodies. */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/lockdep.h>
#include <linux/completion.h>
#define __sched
#define WF_CURRENT_CPU 0x40
#define might_sleep renku_linux_might_sleep
void __prepare_to_swait(struct swait_queue_head *, struct swait_queue *);
void swake_up_all_locked(struct swait_queue_head *);
#endif
