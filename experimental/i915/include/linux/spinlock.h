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
#define raw_spin_lock_init spin_lock_init
#define raw_spin_lock spin_lock
#define raw_spin_unlock spin_unlock
#define raw_spin_lock_irqsave spin_lock_irqsave
#define raw_spin_unlock_irqrestore spin_unlock_irqrestore
void renku_linux_spin_lock_irq(spinlock_t *);
void renku_linux_spin_unlock_irq(spinlock_t *);
#define raw_spin_lock_irq renku_linux_spin_lock_irq
#define raw_spin_unlock_irq renku_linux_spin_unlock_irq
#define __SPIN_LOCK_UNLOCKED __RAW_SPIN_LOCK_UNLOCKED
#ifdef RENKU_NATIVE
#define __RAW_SPIN_LOCK_UNLOCKED(name) { .native = B_SPINLOCK_INITIALIZER, .saved_interrupts = 0 }
#else
#define __RAW_SPIN_LOCK_UNLOCKED(name) { .native = PTHREAD_MUTEX_INITIALIZER }
#endif
/* Native backend startup/shutdown is explicit; lock initialization never sleeps. */
#ifdef RENKU_NATIVE
status_t renku_linux_rw_backend_init(void);
status_t renku_linux_rw_backend_uninit(void);
#endif
void renku_linux_rwlock_init(rwlock_t *);
void renku_linux_read_lock(rwlock_t *);
void renku_linux_read_unlock(rwlock_t *);
void renku_linux_write_lock(rwlock_t *);
void renku_linux_write_unlock(rwlock_t *);
#define rwlock_init renku_linux_rwlock_init
#define read_lock renku_linux_read_lock
#define read_unlock renku_linux_read_unlock
#define write_lock renku_linux_write_lock
#define write_unlock renku_linux_write_unlock
#endif
