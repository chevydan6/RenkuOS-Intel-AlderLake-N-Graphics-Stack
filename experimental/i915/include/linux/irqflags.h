/* SPDX-License-Identifier: MIT */
#ifndef RENKU_IRQFLAGS_H
#define RENKU_IRQFLAGS_H
#include <native_irq_scopes.h>
#define local_irq_save(flags) ((flags) = renku_linux_local_irq_save())
#define local_irq_restore renku_linux_local_irq_restore
#define local_irq_disable() ((void)renku_linux_local_irq_save())
#define local_irq_enable() renku_linux_local_irq_restore(1)
#define irqs_disabled renku_linux_irqs_disabled
#define irqs_disabled_flags(flags) (!(flags))
#endif
