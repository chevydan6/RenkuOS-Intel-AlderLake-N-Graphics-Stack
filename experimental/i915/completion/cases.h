/* SPDX-License-Identifier: MIT */
#ifndef RENKU_COMPLETION_CASES_H
#define RENKU_COMPLETION_CASES_H
#include <linux/completion.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <asm/div64.h>
#include "test.h"
#ifndef RENKU_NATIVE
#include <time.h>
#include <assert.h>
#endif
#define TEST_BUILTIN 1
#define TEST_MOD_MODULE 1
_Static_assert(IS_ENABLED(TEST_BUILTIN) && IS_ENABLED(TEST_MOD) && !IS_ENABLED(TEST_ABSENT), "original kconfig tristates");
_Static_assert(sizeof(__kernel_long_t) == 8 && sizeof(__kernel_old_uid_t) == 2, "original x86-64 UAPI widths");
#define CHECK(r, x) do { (r)->checks++; if (!(x)) { (r)->failures++; \
	if (!(r)->first_line) (r)->first_line = __LINE__; } } while (0)
#include "namespace_cases.h"
#include "bitmap_cases.h"
#include "port_context_cases.h"
#include "cpumask_cases.h"
#ifdef RENKU_NATIVE
#include "preempt_cases.h"
#endif
static s64 NowUS(void) {
#ifdef RENKU_NATIVE
	return system_time();
#else
	return ktime_get() / 1000;
#endif
}
static void PauseUS(unsigned us) {
#ifdef RENKU_NATIVE
	snooze(us);
#else
	struct timespec pause = {.tv_sec = us / 1000000, .tv_nsec = (us % 1000000) * 1000};
	while (nanosleep(&pause, &pause) != 0) {}
#endif
}
static void Fatal(void) { panic("i915 completion test worker lifetime failure"); __builtin_trap(); }
static unsigned Queued(struct completion *completion)
{
	unsigned count = 0;
	unsigned long flags;
	raw_spin_lock_irqsave(&completion->wait.lock, flags);
	struct list_head *node;
	list_for_each(node, &completion->wait.task_list) count++;
	raw_spin_unlock_irqrestore(&completion->wait.lock, flags);
	return count;
}
static bool UntilQueued(struct completion *completion, unsigned count)
{
	s64 deadline = NowUS() + 2000000;
	while (Queued(completion) != count) {
		if (NowUS() >= deadline) return false;
		PauseUS(100);
	}
	return true;
}
static void FinishFixture(struct completion *completion) {
#ifndef RENKU_NATIVE
	assert(pthread_mutex_destroy(&completion->wait.lock.raw_lock.native) == 0);
#else
	(void)completion;
#endif
}
struct test_worker {
	struct completion *target, *ready, *done;
	unsigned mode, *payload;
	long result;
	bool saw_payload;
};
static void WorkerBody(struct test_worker *worker)
{
	struct task_struct task;
	if (!renku_linux_task_register(&task)) Fatal();
	complete(worker->ready);
	switch (worker->mode) {
	case 0: wait_for_completion(worker->target); worker->result = 1; break;
	case 1: worker->result = wait_for_completion_timeout(worker->target, 2000); break;
	case 2: worker->result = wait_for_completion_interruptible_timeout(worker->target, 2000); break;
	case 3: worker->result = wait_for_completion_killable_timeout(worker->target, 2000); break;
	default: worker->result = wait_for_completion_io_timeout(worker->target, 2000); break;
	}
	worker->saw_payload = *worker->payload == 0x91501234;
	renku_linux_task_unregister(&task);
	complete(worker->done);
}
#ifdef RENKU_NATIVE
typedef thread_id worker_id;
static status_t WorkerEntry(void *cookie) { WorkerBody(cookie); return B_OK; }
static worker_id Spawn(struct test_worker *worker)
{
	thread_id id = spawn_kernel_thread(WorkerEntry, "i915 completion waiter", B_NORMAL_PRIORITY, worker);
	if (id < B_OK || resume_thread(id) != B_OK) Fatal();
	return id;
}
static void Join(worker_id id)
{
	status_t returned;
	status_t status = wait_for_thread_etc(id, B_RELATIVE_TIMEOUT, 2000000, &returned);
	if (status == B_BAD_THREAD_ID) {
		thread_info info;
		if (get_thread_info(id, &info) != B_BAD_THREAD_ID) Fatal();
	} else if (status != B_OK || returned != B_OK) Fatal();
}
#else
typedef pthread_t worker_id;
static void *WorkerEntry(void *cookie) { WorkerBody(cookie); return NULL; }
static worker_id Spawn(struct test_worker *worker)
{ pthread_t id; if (pthread_create(&id, NULL, WorkerEntry, worker) != 0) Fatal(); return id; }
static void Join(worker_id id) { if (pthread_join(id, NULL) != 0) Fatal(); }
#endif

#include "raw_lock_cases.h"
#include "lockref_cases.h"
#include "seqlock_cases.h"
#include "page_cases.h"
#include "store_cases.h"
#include "shmem_cases.h"
#ifdef RENKU_NATIVE
#include "rcu_cases.h"
#include "native_page_state_cases.h"
#endif

static void RunCases(struct completion_report *r)
{
	*r = (struct completion_report){.version = 11};
	struct task_struct task;
	if (!renku_linux_task_register(&task)) Fatal();
	NamespaceCases(r);
	BitmapCases(r);
	CPUMaskCases(r);
	RawLayoutCases(r);
	LockrefCases(r);
	ConcurrentLockrefCases(r);
	SeqCases(r);
	ConcurrentSeqCases(r);
	PageCases(r);
	ConcurrentPageCases(r);
	FolioBatchCases(r);
	StoreCases(r);
	ShmemCases(r);
	PortContextCases(r);
#ifdef RENKU_NATIVE
	PreemptCases(r);
	NativeBitCases(r);
	NativeRCUCases(r);
	NativePageStateCases(r);
#endif
	u64 numerator = 100;
	u32 divisor = 7;
	CHECK(r, do_div(numerator, divisor++) == 2 && numerator == 14 && divisor == 8);
	numerator = U64_MAX;
	CHECK(r, do_div(numerator, 0xffffffffu) == 0 && numerator == 0x100000001ULL);
	unsigned long bits[2] = {};
	CHECK(r, !test_and_set_bit(64, bits));
	CHECK(r, test_and_set_bit(64, bits));
	CHECK(r, test_bit_acquire(64, bits) && !test_bit_acquire(63, bits));
	clear_bit_unlock(64, bits); CHECK(r, !test_bit_acquire(64, bits));
	DECLARE_COMPLETION(statically_initialized);
	CHECK(r, !try_wait_for_completion(&statically_initialized));
	complete(&statically_initialized);
	CHECK(r, try_wait_for_completion(&statically_initialized));
	FinishFixture(&statically_initialized);
	struct completion target, ready, done;
	init_completion(&target); init_completion(&ready); init_completion(&done);
	CHECK(r, !completion_done(&target));
	CHECK(r, wait_for_completion_timeout(&target, 0) == 0);
	complete(&target); complete(&target);
	CHECK(r, completion_done(&target));
	CHECK(r, wait_for_completion_timeout(&target, 0) == 1);
	CHECK(r, wait_for_completion_timeout(&target, 0) == 1);
	CHECK(r, !completion_done(&target));
#ifdef RENKU_NATIVE
	extern bool interrupts_enabled(void);
	cpu_status saved = disable_interrupts();
	complete(&target);
	CHECK(r, !interrupts_enabled());
	restore_interrupts(saved);
	CHECK(r, try_wait_for_completion(&target));
#endif
	s64 before = NowUS();
	CHECK(r, wait_for_completion_timeout(&target, 20) == 0);
	CHECK(r, NowUS() - before >= 18000);
	CHECK(r, !swait_active(&target.wait));
	complete_all(&target);
	for (unsigned i = 0; i < 16; i++) CHECK(r, try_wait_for_completion(&target));
	CHECK(r, target.done == UINT_MAX);
	reinit_completion(&target);
	CHECK(r, !try_wait_for_completion(&target));
	if (r->failures) goto cleanup;
	/* Each phase queues two real threads before two individual completions or
	 * a broadcast. Distinct wakeups must not collapse onto the first waiter. */
	for (unsigned phase = 0; phase < 12; phase++) {
		unsigned payload = 0;
		struct test_worker workers[2];
		worker_id ids[2];
		for (unsigned i = 0; i < 2; i++) {
			workers[i] = (struct test_worker){&target, &ready, &done, phase % 5, &payload, 0, false};
			ids[i] = Spawn(&workers[i]); r->workers++;
		}
		for (unsigned i = 0; i < 2; i++)
			if (!wait_for_completion_timeout(&ready, 2000)) Fatal();
		if (!UntilQueued(&target, 2)) Fatal();
		payload = 0x91501234;
		if (phase % 3 == 0) {
#ifdef RENKU_NATIVE
			cpu_status saved = disable_interrupts();
#endif
			complete_all(&target);
#ifdef RENKU_NATIVE
			CHECK(r, !interrupts_enabled());
			restore_interrupts(saved);
#endif
		}
		else if (phase % 3 == 1) {
			complete(&target);
			if (!wait_for_completion_timeout(&done, 2000)) Fatal();
			CHECK(r, Queued(&target) == 1);
			complete(&target);
		} else { complete(&target); complete(&target); }
		for (unsigned i = 0; i < (phase % 3 == 1 ? 1u : 2u); i++)
			if (!wait_for_completion_timeout(&done, 2000)) Fatal();
		for (unsigned i = 0; i < 2; i++) {
			Join(ids[i]);
			CHECK(r, workers[i].result > 0 && workers[i].result <= 2000);
			CHECK(r, workers[i].saw_payload);
		}
		CHECK(r, Queued(&target) == 0);
		if (phase % 3 == 0) CHECK(r, try_wait_for_completion(&target));
		else CHECK(r, !try_wait_for_completion(&target));
		reinit_completion(&target);
		r->phases++;
	}
cleanup:
	FinishFixture(&target); FinishFixture(&ready); FinishFixture(&done);
#ifdef RENKU_NATIVE
	sem_id event = task.wait_event;
#endif
	renku_linux_task_unregister(&task);
	CHECK(r, !task.wait_initialized && task.release_wait == NULL);
#ifdef RENKU_NATIVE
	sem_info info;
	CHECK(r, get_sem_info(event, &info) == B_BAD_SEM_ID);
#endif
}
#endif
