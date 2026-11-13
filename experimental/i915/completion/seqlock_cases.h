/* SPDX-License-Identifier: MIT */
#ifndef RENKU_SEQLOCK_CASES_H
#define RENKU_SEQLOCK_CASES_H
#include <linux/seqlock.h>
static void SeqCases(struct completion_report *r)
{
	DEFINE_SEQLOCK(lock);
	unsigned start = read_seqbegin(&lock);
	CHECK(r, start == 0 && !read_seqretry(&lock, start));
	write_seqlock(&lock);
	CHECK(r, spin_is_locked(&lock.lock) && (raw_read_seqcount(&lock.seqcount) & 1));
#ifdef RENKU_NATIVE
	CHECK(r, !interrupts_enabled() && preempt_count() == 1);
#endif
	write_sequnlock(&lock);
	CHECK(r, !spin_is_locked(&lock.lock) && read_seqretry(&lock, start));
	start = read_seqbegin(&lock);
	CHECK(r, start == 2 && !read_seqretry(&lock, start));
	unsigned long flags;
	write_seqlock_irqsave(&lock, flags);
#ifdef RENKU_NATIVE
	CHECK(r, !interrupts_enabled());
#endif
	write_sequnlock_irqrestore(&lock, flags);
	CHECK(r, read_seqretry(&lock, start));
#ifdef RENKU_NATIVE
	CHECK(r, preemptible() && interrupts_enabled());
	preempt_disable();
	write_seqlock_irqsave(&lock, flags);
	CHECK(r, preempt_count() == 2 && !interrupts_enabled());
	write_sequnlock_irqrestore(&lock, flags);
	CHECK(r, preempt_count() == 1 && !interrupts_enabled());
	preempt_enable();
#endif
	write_seqlock_irq(&lock); write_sequnlock_irq(&lock);
	CHECK(r, !spin_is_locked(&lock.lock));
#ifdef RENKU_NATIVE
	CHECK(r, preemptible() && interrupts_enabled());
#endif
	start = read_seqbegin(&lock);
	read_seqlock_excl(&lock);
	CHECK(r, spin_is_locked(&lock.lock));
	read_sequnlock_excl(&lock);
	CHECK(r, !read_seqretry(&lock, start)); /* exclusive reader never writes sequence */
	int mode = 1;
	read_seqbegin_or_lock(&lock, &mode);
	CHECK(r, spin_is_locked(&lock.lock) && !need_seqretry(&lock, mode));
	done_seqretry(&lock, mode);
	CHECK(r, !spin_is_locked(&lock.lock));
	mode = 0; read_seqbegin_or_lock(&lock, &mode);
	CHECK(r, !need_seqretry(&lock, mode));
	write_seqlock(&lock); write_sequnlock(&lock);
	CHECK(r, need_seqretry(&lock, mode));
	done_seqretry(&lock, mode);
	seqcount_t raw;
	seqcount_init(&raw);
	spin_lock(&lock.lock); /* caller serializes and excludes native preemption */
	start = read_seqcount_begin(&raw);
	write_seqcount_begin(&raw);
	CHECK(r, raw_read_seqcount(&raw) == 1);
	write_seqcount_end(&raw);
	CHECK(r, raw.sequence == 2 && read_seqcount_retry(&raw, start));
	start = read_seqcount_begin(&raw); write_seqcount_invalidate(&raw);
	CHECK(r, raw.sequence == 4 && read_seqcount_retry(&raw, start));
	raw.sequence = UINT_MAX - 1;
	start = read_seqcount_begin(&raw);
	write_seqcount_begin(&raw);
	CHECK(r, raw_read_seqcount(&raw) == UINT_MAX);
	write_seqcount_end(&raw);
	CHECK(r, raw.sequence == 0 && read_seqcount_retry(&raw, start));
	start = read_seqcount_begin(&raw);
	CHECK(r, start == 0 && !read_seqcount_retry(&raw, start));
	seqcount_spinlock_t associated;
	seqcount_spinlock_init(&associated, &lock.lock);
	write_seqcount_begin(&associated); write_seqcount_end(&associated);
	CHECK(r, read_seqcount_begin(&associated) == 2);
	spin_unlock(&lock.lock);
#ifdef RENKU_NATIVE
	struct mutex mutex;
	mutex_init(&mutex);
	seqcount_mutex_t mutex_count;
	seqcount_mutex_init(&mutex_count, &mutex);
	mutex_lock(&mutex);
	write_seqcount_begin(&mutex_count);
	CHECK(r, preempt_count() == 1 && !interrupts_enabled());
	write_seqcount_end(&mutex_count);
	CHECK(r, preemptible() && read_seqcount_begin(&mutex_count) == 2);
	mutex_unlock(&mutex); mutex_destroy(&mutex);
#else
	CHECK(r, pthread_mutex_destroy(&lock.lock.rlock.raw_lock.native) == 0);
#endif
}
struct seq_fixture {
	seqlock_t lock;
	struct completion ready, gate, done;
	unsigned value, inverse, snapshot_ready, first_write_done, forced_retry, errors, reads;
};
struct seq_arg { struct seq_fixture *f; bool writer; };
static void SeqWait(unsigned *flag)
{
	s64 deadline = NowUS() + 2000000;
	while (!__atomic_load_n(flag, __ATOMIC_ACQUIRE)) {
		if (NowUS() >= deadline) Fatal();
		PauseUS(100); /* task context, outside every write/IRQ scope */
	}
}
static void SeqWorkerBody(struct seq_arg *arg)
{
	struct seq_fixture *f = arg->f;
	struct task_struct task;
	if (!renku_linux_task_register(&task)) Fatal();
	complete(&f->ready); wait_for_completion(&f->gate);
	if (arg->writer) {
		SeqWait(&f->snapshot_ready);
		for (unsigned i = 1; i <= 4000; i++) {
			write_seqlock(&f->lock);
			WRITE_ONCE(f->value, i);
			for (unsigned pause = 0; pause < 64; pause++) cpu_relax();
			WRITE_ONCE(f->inverse, ~i);
			write_sequnlock(&f->lock);
			if (i == 1) __atomic_store_n(&f->first_write_done, 1, __ATOMIC_RELEASE);
		}
	} else {
		/* Require an actual read/write overlap and rejected old sequence,
		 * independently of stress scheduling. Fixture lifetime is owned until
		 * both joins; there are no reclaimable pointers in the snapshot. */
		unsigned old = read_seqbegin(&f->lock);
		__atomic_store_n(&f->snapshot_ready, 1, __ATOMIC_RELEASE);
		SeqWait(&f->first_write_done);
		if (read_seqretry(&f->lock, old)) f->forced_retry = 1;
		else __atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
		for (unsigned i = 0; i < 4000; i++) {
			unsigned start, value, inverse;
			do {
				start = read_seqbegin(&f->lock);
				value = READ_ONCE(f->value); inverse = READ_ONCE(f->inverse);
			} while (read_seqretry(&f->lock, start));
			if (inverse != ~value) __atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
			f->reads++;
		}
	}
	renku_linux_task_unregister(&task); complete(&f->done);
}
#ifdef RENKU_NATIVE
static status_t SeqWorker(void *cookie) { SeqWorkerBody(cookie); return B_OK; }
#else
static void *SeqWorker(void *cookie) { SeqWorkerBody(cookie); return NULL; }
#endif
static void ConcurrentSeqCases(struct completion_report *r)
{
	struct seq_fixture f = {.inverse = ~0u};
	seqlock_init(&f.lock);
	init_completion(&f.ready); init_completion(&f.gate); init_completion(&f.done);
	struct seq_arg args[] = {{&f, true}, {&f, false}};
	worker_id ids[2];
	for (unsigned i = 0; i < 2; i++) {
#ifdef RENKU_NATIVE
		ids[i] = spawn_kernel_thread(SeqWorker, "i915 original seqlock", B_NORMAL_PRIORITY, &args[i]);
		if (ids[i] < B_OK || resume_thread(ids[i]) != B_OK) Fatal();
#else
		if (pthread_create(&ids[i], NULL, SeqWorker, &args[i])) Fatal();
#endif
	}
	for (unsigned i = 0; i < 2; i++) if (!wait_for_completion_timeout(&f.ready, 2000)) Fatal();
	if (!UntilQueued(&f.gate, 2)) Fatal();
	complete_all(&f.gate);
	for (unsigned i = 0; i < 2; i++) {
		if (!wait_for_completion_timeout(&f.done, 2000)) Fatal();
		Join(ids[i]);
	}
	CHECK(r, f.value == 4000 && f.inverse == ~4000u && f.reads == 4000);
	CHECK(r, f.errors == 0 && f.forced_retry == 1);
	CHECK(r, read_seqbegin(&f.lock) == 8000 && !spin_is_locked(&f.lock.lock));
#ifdef RENKU_NATIVE
	CHECK(r, preemptible() && interrupts_enabled());
#else
	CHECK(r, pthread_mutex_destroy(&f.lock.lock.rlock.raw_lock.native) == 0);
#endif
	r->seq_workers = 2; r->seq_writes = f.value; r->seq_reads = f.reads; r->seq_forced_retries = f.forced_retry;
	FinishFixture(&f.ready); FinishFixture(&f.gate); FinishFixture(&f.done);
}
#endif
