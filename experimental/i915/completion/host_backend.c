/* SPDX-License-Identifier: MIT */
/* Real pthread locks, with thread-local IRQ state modeled only for CPU tests. */
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
static _Thread_local bool enabled = true;
void panic(const char *format, ...)
{ va_list args; va_start(args, format); vfprintf(stderr, format, args); va_end(args); abort(); }
bool renku_kpi_warn(const char *format, ...) { (void)format; abort(); }
void renku_linux_raw_spin_init(raw_spinlock_t *l) { assert(!pthread_mutex_init(&l->raw_lock.native, NULL)); }
unsigned long renku_linux_raw_spin_lock_irqsave(raw_spinlock_t *l)
{ bool old = enabled; enabled = false; assert(!pthread_mutex_lock(&l->raw_lock.native)); return old; }
void renku_linux_raw_spin_unlock_irqrestore(raw_spinlock_t *l, unsigned long flags)
{ assert(!pthread_mutex_unlock(&l->raw_lock.native)); enabled = flags; }
void renku_linux_raw_spin_lock(raw_spinlock_t *l) { l->raw_lock.saved_interrupts = renku_linux_raw_spin_lock_irqsave(l); }
void renku_linux_raw_spin_unlock(raw_spinlock_t *l)
{ unsigned long old = l->raw_lock.saved_interrupts; renku_linux_raw_spin_unlock_irqrestore(l, old); }
void renku_linux_raw_spin_lock_irq(raw_spinlock_t *l)
{ enabled = false; assert(!pthread_mutex_lock(&l->raw_lock.native)); }
void renku_linux_raw_spin_unlock_irq(raw_spinlock_t *l)
{ assert(!pthread_mutex_unlock(&l->raw_lock.native)); enabled = true; }

void renku_linux_log_emit(char level, const char *text)
{ fprintf(stderr, "[i915-linux %c] %s", level, text); }

bool renku_linux_spin_is_locked(spinlock_t *lock)
{
	int status = pthread_mutex_trylock(&lock->rlock.raw_lock.native);
	if (status == EBUSY) return true;
	if (status != 0) panic("host spin query failed: %d", status);
	if (pthread_mutex_unlock(&lock->rlock.raw_lock.native)) panic("host spin query unlock");
	return false;
}

/* Explicit host model of the native non-preemptible/IRQ scopes. */
static _Thread_local unsigned preempt_depth;
static _Thread_local bool preempt_saved;
void renku_linux_preempt_disable(void)
{
	if (!preempt_depth++) preempt_saved = enabled;
	enabled = false;
}
void renku_linux_preempt_enable(void)
{
	assert(preempt_depth);
	if (!--preempt_depth) enabled = preempt_saved;
}
unsigned int renku_linux_preempt_count(void) { return preempt_depth; }
bool renku_linux_preemptible(void) { return enabled && !preempt_depth; }

void renku_linux_mutex_init_named(struct mutex *m, const char *name, struct lock_class_key *key)
{
	(void)name; (void)key;
	m->held = 0;
	assert(pthread_mutex_init(&m->native, NULL) == 0);
}
void renku_linux_mutex_init(struct mutex *m) { renku_linux_mutex_init_named(m, NULL, NULL); }
void renku_linux_mutex_destroy(struct mutex *m) { assert(pthread_mutex_destroy(&m->native) == 0); }
void renku_linux_mutex_lock(struct mutex *m)
{
	assert(pthread_mutex_lock(&m->native) == 0);
	__atomic_store_n(&m->held, 1, __ATOMIC_RELEASE);
}
void renku_linux_mutex_unlock(struct mutex *m)
{
	__atomic_store_n(&m->held, 0, __ATOMIC_RELEASE);
	assert(pthread_mutex_unlock(&m->native) == 0);
}
