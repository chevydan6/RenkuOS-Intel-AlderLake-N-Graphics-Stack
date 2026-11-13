/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_SPINLOCK_H
#define RENKU_LINUX_SPINLOCK_H
#include <linux/spinlock_types.h>
void renku_linux_raw_spin_init(raw_spinlock_t *);
void renku_linux_raw_spin_lock(raw_spinlock_t *);
void renku_linux_raw_spin_unlock(raw_spinlock_t *);
unsigned long renku_linux_raw_spin_lock_irqsave(raw_spinlock_t *);
void renku_linux_raw_spin_unlock_irqrestore(raw_spinlock_t *, unsigned long);
void renku_linux_raw_spin_lock_irq(raw_spinlock_t *);
void renku_linux_raw_spin_unlock_irq(raw_spinlock_t *);
#define raw_spin_lock_init renku_linux_raw_spin_init
#define raw_spin_lock renku_linux_raw_spin_lock
#define raw_spin_unlock renku_linux_raw_spin_unlock
#define raw_spin_lock_irqsave(lock, flags) ((flags) = renku_linux_raw_spin_lock_irqsave(lock))
#define raw_spin_unlock_irqrestore renku_linux_raw_spin_unlock_irqrestore
#define raw_spin_lock_irq renku_linux_raw_spin_lock_irq
#define raw_spin_unlock_irq renku_linux_raw_spin_unlock_irq
/* Linux's original non-RT wrapper contains a distinct raw-spinlock object. */
static inline void renku_linux_spin_init(spinlock_t *l) { raw_spin_lock_init(&l->rlock); }
static inline void renku_linux_spin_lock(spinlock_t *l) { raw_spin_lock(&l->rlock); }
static inline void renku_linux_spin_unlock(spinlock_t *l) { raw_spin_unlock(&l->rlock); }
static inline unsigned long renku_linux_spin_lock_irqsave(spinlock_t *l)
{ return renku_linux_raw_spin_lock_irqsave(&l->rlock); }
static inline void renku_linux_spin_unlock_irqrestore(spinlock_t *l, unsigned long f)
{ raw_spin_unlock_irqrestore(&l->rlock, f); }
bool renku_linux_spin_is_locked(spinlock_t *);
#define spin_is_locked renku_linux_spin_is_locked
#define assert_spin_locked(l) BUG_ON(!spin_is_locked(l))
#define spin_lock_init renku_linux_spin_init
#define spin_lock renku_linux_spin_lock
#define spin_unlock renku_linux_spin_unlock
#define spin_lock_irqsave(lock, flags) ((flags) = renku_linux_spin_lock_irqsave(lock))
#define spin_unlock_irqrestore renku_linux_spin_unlock_irqrestore
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
