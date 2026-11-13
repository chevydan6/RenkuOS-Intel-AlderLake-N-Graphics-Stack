/* SPDX-License-Identifier: MIT */
#ifndef RENKU_NATIVE_IRQ_SCOPES_H
#define RENKU_NATIVE_IRQ_SCOPES_H
#include <kpi_base.h>
#ifdef RENKU_NATIVE
status_t renku_linux_irq_backend_init(void);
status_t renku_linux_irq_backend_uninit(void);
#endif
void renku_linux_irq_mask_enter(bool preempt);
void renku_linux_irq_mask_leave(bool preempt);
unsigned int renku_linux_irq_mask_count(void);
unsigned long renku_linux_local_irq_save(void);
void renku_linux_local_irq_restore(unsigned long);
bool renku_linux_irqs_disabled(void);
#endif
