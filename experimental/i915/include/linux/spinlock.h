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
