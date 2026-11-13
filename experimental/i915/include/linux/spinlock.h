/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_SPINLOCK_H
#define RENKU_LINUX_SPINLOCK_H
#include <linux/spinlock_types.h>
void renku_linux_spin_init(spinlock_t *);
void renku_linux_spin_lock(spinlock_t *);
void renku_linux_spin_unlock(spinlock_t *);
unsigned long renku_linux_spin_lock_irqsave(spinlock_t *);
void renku_linux_spin_unlock_irqrestore(spinlock_t *, unsigned long);
#define spin_lock_init renku_linux_spin_init
#define spin_lock renku_linux_spin_lock
#define spin_unlock renku_linux_spin_unlock
#define spin_lock_irqsave(lock, flags) ((flags) = renku_linux_spin_lock_irqsave(lock))
#define spin_unlock_irqrestore renku_linux_spin_unlock_irqrestore
#endif
