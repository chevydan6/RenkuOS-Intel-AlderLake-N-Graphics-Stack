/* SPDX-License-Identifier: MIT */
/* Real host mutex backend for CPU tests; does not validate native adapters. */
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <assert.h>
#include <stdlib.h>
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
void renku_linux_spin_init(spinlock_t *s) { assert(pthread_mutex_init(&s->native, NULL) == 0); }
void renku_linux_spin_lock(spinlock_t *s) { assert(pthread_mutex_lock(&s->native) == 0); }
void renku_linux_spin_unlock(spinlock_t *s) { assert(pthread_mutex_unlock(&s->native) == 0); }
/* IRQ execution is unavailable in userspace; never manufacture saved flags. */
unsigned long renku_linux_spin_lock_irqsave(spinlock_t *s) { (void)s; abort(); }
void renku_linux_spin_unlock_irqrestore(spinlock_t *s, unsigned long f) { (void)s; (void)f; abort(); }
