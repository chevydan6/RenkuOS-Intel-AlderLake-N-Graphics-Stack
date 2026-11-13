/* SPDX-License-Identifier: MIT */
/* Real pthread locks, with thread-local IRQ state modeled only for CPU tests. */
#include <linux/spinlock.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
static _Thread_local bool enabled = true;
void panic(const char *format, ...)
{ va_list args; va_start(args, format); vfprintf(stderr, format, args); va_end(args); abort(); }
bool renku_kpi_warn(const char *format, ...) { (void)format; abort(); }
void renku_linux_spin_init(spinlock_t *l) { assert(!pthread_mutex_init(&l->native, NULL)); }
unsigned long renku_linux_spin_lock_irqsave(spinlock_t *l)
{ bool old = enabled; enabled = false; assert(!pthread_mutex_lock(&l->native)); return old; }
void renku_linux_spin_unlock_irqrestore(spinlock_t *l, unsigned long flags)
{ assert(!pthread_mutex_unlock(&l->native)); enabled = flags; }
void renku_linux_spin_lock(spinlock_t *l) { l->saved_interrupts = renku_linux_spin_lock_irqsave(l); }
void renku_linux_spin_unlock(spinlock_t *l)
{ unsigned long old = l->saved_interrupts; renku_linux_spin_unlock_irqrestore(l, old); }
void renku_linux_spin_lock_irq(spinlock_t *l)
{ enabled = false; assert(!pthread_mutex_lock(&l->native)); }
void renku_linux_spin_unlock_irq(spinlock_t *l)
{ assert(!pthread_mutex_unlock(&l->native)); enabled = true; }
