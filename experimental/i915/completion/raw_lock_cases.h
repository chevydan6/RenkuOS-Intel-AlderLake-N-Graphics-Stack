/* SPDX-License-Identifier: MIT */
#ifndef RENKU_RAW_LOCK_CASES_H
#define RENKU_RAW_LOCK_CASES_H
#include <linux/bit_spinlock.h>
static void RawLayoutCases(struct completion_report *r)
{
	DEFINE_RAW_SPINLOCK(raw);
	DEFINE_SPINLOCK(regular);
	_Static_assert(!__same_type(raw, regular), "original raw/regular types are distinct");
	_Static_assert(offsetof(spinlock_t, rlock) == 0, "original regular wrapper");
	_Static_assert(sizeof(raw_spinlock_t) == sizeof(arch_spinlock_t), "non-debug raw wrapper");
	raw_spin_lock(&raw);
	spin_lock(&regular);
#ifdef RENKU_NATIVE
	CHECK(r, preempt_count() == 2 && !interrupts_enabled());
#else
	CHECK(r, sizeof(raw.raw_lock.native) == sizeof(pthread_mutex_t));
#endif
	spin_unlock(&regular);
	raw_spin_unlock(&raw);
#ifdef RENKU_NATIVE
	CHECK(r, preempt_count() == 0 && interrupts_enabled());
	DEFINE_RWLOCK(rw);
	read_lock(&rw); CHECK(r, !interrupts_enabled()); read_unlock(&rw);
	write_lock(&rw); CHECK(r, !interrupts_enabled()); write_unlock(&rw);
#else
	CHECK(r, pthread_mutex_destroy(&regular.rlock.raw_lock.native) == 0);
	CHECK(r, pthread_mutex_destroy(&raw.raw_lock.native) == 0);
#endif
}
#ifdef RENKU_NATIVE
struct bit_fixture {
	struct completion ready, gate, done;
	unsigned long flags;
	unsigned value, mirror, errors;
};
static status_t BitWorker(void *cookie)
{
	struct bit_fixture *f = cookie;
	struct task_struct task;
	if (!renku_linux_task_register(&task)) Fatal();
	complete(&f->ready);
	wait_for_completion(&f->gate);
	for (unsigned i = 0; i < 4000; i++) {
		bit_spin_lock(0, &f->flags);
		if (f->mirror != ~f->value) f->errors++;
		f->value++;
		f->mirror = ~f->value;
		__set_bit(17, &f->flags); /* all flags in the word share this lock */
		if (i & 1) __bit_spin_unlock(0, &f->flags);
		else bit_spin_unlock(0, &f->flags);
	}
	renku_linux_task_unregister(&task);
	complete(&f->done);
	return B_OK;
}
static void NativeBitCases(struct completion_report *r)
{
	unsigned long flags = BIT(17);
	CHECK(r, !bit_spin_is_locked(0, &flags));
	bit_spin_lock(0, &flags);
	CHECK(r, bit_spin_is_locked(0, &flags) && preempt_count() == 1 && !interrupts_enabled());
	CHECK(r, !bit_spin_trylock(0, &flags) && preempt_count() == 1);
	bit_spin_unlock(0, &flags);
	CHECK(r, flags == BIT(17) && preempt_count() == 0 && interrupts_enabled());
	CHECK(r, bit_spin_trylock(0, &flags) && preempt_count() == 1);
	__set_bit(23, &flags);
	__bit_spin_unlock(0, &flags);
	CHECK(r, flags == (BIT(17) | BIT(23)) && preemptible());
	preempt_disable();
	bit_spin_lock(0, &flags);
	CHECK(r, preempt_count() == 2);
	bit_spin_unlock(0, &flags);
	CHECK(r, preempt_count() == 1 && !interrupts_enabled());
	preempt_enable();
	struct bit_fixture f = {.flags = BIT(17), .mirror = ~0u};
	init_completion(&f.ready); init_completion(&f.gate); init_completion(&f.done);
	thread_id ids[2];
	for (unsigned i = 0; i < 2; i++) {
		ids[i] = spawn_kernel_thread(BitWorker, "i915 original bit lock", B_NORMAL_PRIORITY, &f);
		if (ids[i] < B_OK || resume_thread(ids[i]) != B_OK) Fatal();
	}
	for (unsigned i = 0; i < 2; i++) if (!wait_for_completion_timeout(&f.ready, 2000)) Fatal();
	if (!UntilQueued(&f.gate, 2)) Fatal();
	complete_all(&f.gate);
	for (unsigned i = 0; i < 2; i++) {
		if (!wait_for_completion_timeout(&f.done, 2000)) Fatal();
		Join(ids[i]);
	}
	CHECK(r, f.value == 8000 && f.mirror == ~f.value && f.errors == 0);
	CHECK(r, f.flags == BIT(17) && preempt_count() == 0 && interrupts_enabled());
	CHECK(r, !Queued(&f.ready) && !Queued(&f.gate) && !Queued(&f.done));
	r->bit_workers = 2; r->bit_iterations = f.value;
}
#endif
#endif
