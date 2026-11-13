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
