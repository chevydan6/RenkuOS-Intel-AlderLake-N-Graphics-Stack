/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_PREEMPT_H
#define RENKU_LINUX_PREEMPT_H
#include <kpi_base.h>
/* Scoped task-context preemption exclusion. Native IRQ exclusion is stronger
 * than Linux preempt_disable; callers must keep these regions short/nonblocking.
 * Linux hardirq/softirq/NMI counter encodings are not synthesized here. */
#ifdef RENKU_NATIVE
status_t renku_linux_preempt_backend_init(void);
status_t renku_linux_preempt_backend_uninit(void);
#endif
void renku_linux_preempt_disable(void);
void renku_linux_preempt_enable(void);
unsigned int renku_linux_preempt_count(void);
bool renku_linux_preemptible(void);
bool renku_linux_in_atomic(void);
#define preempt_disable renku_linux_preempt_disable
#define preempt_disable_notrace renku_linux_preempt_disable
#define preempt_enable renku_linux_preempt_enable
#define preempt_enable_notrace renku_linux_preempt_enable
#define preempt_count renku_linux_preempt_count
#define preemptible renku_linux_preemptible
#define in_atomic renku_linux_in_atomic
/* Interrupt-context classification needs the actual driver IRQ entry path.
 * Keep it unresolved rather than reporting fictitious task/IRQ context. */
int in_interrupt(void);
int in_hardirq(void);
int in_nmi(void);
#endif
