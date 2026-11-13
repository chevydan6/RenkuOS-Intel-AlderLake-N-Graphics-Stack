/* SPDX-License-Identifier: MIT */
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <native_irq_scopes.h>
#ifndef RENKU_NATIVE
#error This adapter requires real RenkuOS kernel interfaces
#endif

void renku_linux_mutex_init(struct mutex *lock)
{
	renku_linux_mutex_init_named(lock, "i915 LinuxKPI mutex", NULL);
}
void renku_linux_mutex_init_named(struct mutex *lock, const char *name, struct lock_class_key *key)
{
	(void)key; /* Lockdep disabled; the real semaphore still carries its name. */
	lock->held = 0;
	lock->semaphore = create_sem(1, name);
	if (lock->semaphore < B_OK) {
		panic("i915: create mutex: %d", lock->semaphore);
		__builtin_trap();
	}
}
void renku_linux_mutex_destroy(struct mutex *lock)
{
	status_t error = delete_sem(lock->semaphore);
	if (error != B_OK) { panic("i915: delete mutex: %d", error); __builtin_trap(); }
}
void renku_linux_mutex_lock(struct mutex *lock)
{
	status_t error;
	do { error = acquire_sem(lock->semaphore); } while (error == B_INTERRUPTED);
	if (error != B_OK) { panic("i915: lock mutex: %d", error); __builtin_trap(); }
	__atomic_store_n(&lock->held, 1, __ATOMIC_RELEASE);
}
void renku_linux_mutex_unlock(struct mutex *lock)
{
	/* Clear before waking another owner, who can immediately set it again. */
	__atomic_store_n(&lock->held, 0, __ATOMIC_RELEASE);
	status_t error = release_sem(lock->semaphore);
	if (error != B_OK) { panic("i915: unlock mutex: %d", error); __builtin_trap(); }
}
void renku_linux_raw_spin_init(raw_spinlock_t *lock)
{
	B_INITIALIZE_SPINLOCK(&lock->raw_lock.native);
}
unsigned long renku_linux_raw_spin_lock_irqsave(raw_spinlock_t *lock)
{
	cpu_status saved = renku_linux_local_irq_save();
	renku_linux_irq_mask_enter(false);
	acquire_spinlock(&lock->raw_lock.native);
	return saved;
}
void renku_linux_raw_spin_unlock_irqrestore(raw_spinlock_t *lock, unsigned long saved)
{
	release_spinlock(&lock->raw_lock.native);
	renku_linux_irq_mask_leave(false);
	renku_linux_local_irq_restore(saved);
}
void renku_linux_raw_spin_lock(raw_spinlock_t *lock)
{
	renku_linux_irq_mask_enter(false);
	acquire_spinlock(&lock->raw_lock.native);
}
void renku_linux_raw_spin_unlock(raw_spinlock_t *lock)
{
	release_spinlock(&lock->raw_lock.native);
	renku_linux_irq_mask_leave(false);
}
void renku_linux_raw_spin_lock_irq(raw_spinlock_t *lock)
{
	renku_linux_local_irq_save();
	renku_linux_irq_mask_enter(false);
	acquire_spinlock(&lock->raw_lock.native);
}
void renku_linux_raw_spin_unlock_irq(raw_spinlock_t *lock)
{
	release_spinlock(&lock->raw_lock.native);
	renku_linux_irq_mask_leave(false);
	/* Request IRQ enable, deferred until all native exclusion scopes end. */
	renku_linux_local_irq_restore(1);
}
