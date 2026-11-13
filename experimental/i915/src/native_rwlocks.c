/* SPDX-License-Identifier: MIT */
#include <linux/spinlock.h>
#include <native_irq_scopes.h>
#ifndef RENKU_NATIVE
#error This adapter requires the native kernel
#endif
status_t renku_linux_rw_backend_init(void) { return renku_linux_irq_backend_init(); }
status_t renku_linux_rw_backend_uninit(void) { return renku_linux_irq_backend_uninit(); }
void renku_linux_rwlock_init(rwlock_t *lock) { B_INITIALIZE_RW_SPINLOCK(&lock->native); }
void renku_linux_read_lock(rwlock_t *lock)
{ renku_linux_irq_mask_enter(false); acquire_read_spinlock(&lock->native); }
void renku_linux_read_unlock(rwlock_t *lock)
{ release_read_spinlock(&lock->native); renku_linux_irq_mask_leave(false); }
void renku_linux_write_lock(rwlock_t *lock)
{ renku_linux_irq_mask_enter(false); acquire_write_spinlock(&lock->native); }
void renku_linux_write_unlock(rwlock_t *lock)
{ release_write_spinlock(&lock->native); renku_linux_irq_mask_leave(false); }
