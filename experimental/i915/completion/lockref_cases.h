/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LOCKREF_CASES_H
#define RENKU_LOCKREF_CASES_H
#include <linux/lockref.h>
#include <linux/gfp.h>
static void LockrefCases(struct completion_report *r)
{
	struct lockref ref;
	spin_lock_init(&ref.lock);
	const int counts[] = {-128, -1, 0, 1, 2, 3, 42};
	for (unsigned i = 0; i < sizeof(counts)/sizeof(counts[0]); i++) {
		int initial = counts[i];
		ref.count = initial;
		CHECK(r, !spin_is_locked(&ref.lock));
		CHECK(r, lockref_get_not_zero(&ref) == (initial > 0));
		CHECK(r, ref.count == initial + (initial > 0));
		ref.count = initial;
		CHECK(r, lockref_get_not_dead(&ref) == (initial >= 0));
		CHECK(r, ref.count == initial + (initial >= 0));
		ref.count = initial;
		CHECK(r, lockref_put_not_zero(&ref) == (initial > 1));
		CHECK(r, ref.count == initial - (initial > 1));
		/* Original non-cmpxchg fallback reports retry; it does not decrement. */
		ref.count = initial;
		CHECK(r, lockref_put_return(&ref) == -1 && ref.count == initial);
		CHECK(r, lockref_put_or_lock(&ref) == (initial > 1));
		CHECK(r, ref.count == initial - (initial > 1));
		CHECK(r, spin_is_locked(&ref.lock) == (initial <= 1));
#ifdef RENKU_NATIVE
		CHECK(r, preempt_count() == (unsigned)(initial <= 1));
		CHECK(r, interrupts_enabled() == (initial > 1));
#endif
		if (initial <= 1) spin_unlock(&ref.lock);
		CHECK(r, !spin_is_locked(&ref.lock));
		if (initial > 0) {
			ref.count = initial;
			lockref_get(&ref);
			CHECK(r, ref.count == initial + 1);
		}
	}
	ref.count = 1;
	spin_lock(&ref.lock);
	CHECK(r, !__lockref_is_dead(&ref));
	lockref_mark_dead(&ref);
	CHECK(r, __lockref_is_dead(&ref) && ref.count == -128);
	spin_unlock(&ref.lock);
	CHECK(r, !lockref_get_not_dead(&ref) && !lockref_get_not_zero(&ref));
	CHECK(r, !lockref_put_not_zero(&ref) && ref.count == -128);
#ifdef RENKU_NATIVE
	CHECK(r, preemptible() && interrupts_enabled());
#else
	CHECK(r, pthread_mutex_destroy(&ref.lock.rlock.raw_lock.native) == 0);
#endif
	const gfp_t flags[] = {0, GFP_NOWAIT, GFP_ATOMIC, GFP_KERNEL, GFP_KERNEL_ACCOUNT,
		GFP_NOIO, GFP_NOFS, GFP_HIGHUSER, __GFP_DIRECT_RECLAIM, __GFP_ZERO, __GFP_DMA32};
	const bool blocking[] = {false, false, false, true, true, true, true, true, true, false, false};
	for (unsigned i = 0; i < sizeof(flags)/sizeof(flags[0]); i++)
		CHECK(r, gfpflags_allow_blocking(flags[i]) == blocking[i]);
}
struct lockref_fixture {
	struct lockref ref;
	struct completion ready, gate, done;
	unsigned errors;
};
static void LockrefWorkerBody(struct lockref_fixture *f)
{
	struct task_struct task;
	if (!renku_linux_task_register(&task)) Fatal();
	complete(&f->ready);
	wait_for_completion(&f->gate);
	for (unsigned i = 0; i < 4000; i++) {
		if (!lockref_get_not_zero(&f->ref)) __atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
		if (!lockref_put_not_zero(&f->ref)) __atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
		lockref_get(&f->ref);
		if (!lockref_put_or_lock(&f->ref)) {
			__atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
			spin_unlock(&f->ref.lock);
		}
	}
	renku_linux_task_unregister(&task);
	complete(&f->done);
}
#ifdef RENKU_NATIVE
static status_t LockrefWorker(void *cookie) { LockrefWorkerBody(cookie); return B_OK; }
#else
static void *LockrefWorker(void *cookie) { LockrefWorkerBody(cookie); return NULL; }
#endif
static void ConcurrentLockrefCases(struct completion_report *r)
{
	struct lockref_fixture f = {0};
	spin_lock_init(&f.ref.lock); f.ref.count = 1; /* owner retains the base reference */
	init_completion(&f.ready); init_completion(&f.gate); init_completion(&f.done);
	worker_id ids[2];
	for (unsigned i = 0; i < 2; i++) {
#ifdef RENKU_NATIVE
		ids[i] = spawn_kernel_thread(LockrefWorker, "i915 original lockref", B_NORMAL_PRIORITY, &f);
		if (ids[i] < B_OK || resume_thread(ids[i]) != B_OK) Fatal();
#else
		if (pthread_create(&ids[i], NULL, LockrefWorker, &f)) Fatal();
#endif
	}
	for (unsigned i = 0; i < 2; i++) if (!wait_for_completion_timeout(&f.ready, 2000)) Fatal();
	if (!UntilQueued(&f.gate, 2)) Fatal();
	complete_all(&f.gate);
	for (unsigned i = 0; i < 2; i++) {
		if (!wait_for_completion_timeout(&f.done, 2000)) Fatal();
		Join(ids[i]);
	}
	CHECK(r, f.errors == 0 && f.ref.count == 1);
	CHECK(r, !spin_is_locked(&f.ref.lock));
	CHECK(r, !Queued(&f.ready) && !Queued(&f.gate) && !Queued(&f.done));
#ifdef RENKU_NATIVE
	CHECK(r, preemptible() && interrupts_enabled());
#else
	CHECK(r, pthread_mutex_destroy(&f.ref.lock.rlock.raw_lock.native) == 0);
#endif
	r->lockref_workers = 2; r->lockref_iterations = 8000;
	FinishFixture(&f.ready); FinishFixture(&f.gate); FinishFixture(&f.done);
}
#endif
