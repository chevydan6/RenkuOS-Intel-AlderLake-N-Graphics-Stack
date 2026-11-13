/* SPDX-License-Identifier: MIT */
#include <linux/preempt.h>
#include <native_irq_scopes.h>
status_t renku_linux_preempt_backend_init(void) { return renku_linux_irq_backend_init(); }
status_t renku_linux_preempt_backend_uninit(void) { return renku_linux_irq_backend_uninit(); }
void renku_linux_preempt_disable(void) { renku_linux_irq_mask_enter(true); }
void renku_linux_preempt_enable(void) { renku_linux_irq_mask_leave(true); }
unsigned int renku_linux_preempt_count(void) { return renku_linux_irq_mask_count(); }
bool renku_linux_preemptible(void)
{ return !renku_linux_irqs_disabled() && renku_linux_preempt_count() == 0; }
bool renku_linux_in_atomic(void) { return !renku_linux_preemptible(); }
