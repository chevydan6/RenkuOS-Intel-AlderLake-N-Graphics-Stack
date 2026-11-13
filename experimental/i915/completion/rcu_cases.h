/* SPDX-License-Identifier: MIT */
#ifndef RENKU_RCU_CASES_H
#define RENKU_RCU_CASES_H
#include <linux/rcupdate.h>
#include <linux/slab.h>
struct rcu_node { unsigned value; };
struct rcu_fixture {
	struct rcu_node *published;
	struct completion done;
	unsigned ready, nested_request, nested_done, release_reader;
	unsigned writer_started, writer_done, errors;
};
static bool SpinUntil(unsigned *flag, bigtime_t deadline)
{
	while (!__atomic_load_n(flag, __ATOMIC_ACQUIRE)) {
		if (system_time() >= deadline) return false;
		cpu_relax();
	}
	return true;
}
static bool SleepUntil(unsigned *flag)
{
	bigtime_t deadline = system_time() + 2000000;
	while (!__atomic_load_n(flag, __ATOMIC_ACQUIRE)) {
		if (system_time() >= deadline) return false;
		snooze(100);
	}
	return true;
}
static status_t RCUReader(void *cookie)
{
	struct rcu_fixture *f = cookie;
	struct task_struct task;
	if (!renku_linux_task_register(&task)) Fatal();
	rcu_read_lock();
	struct rcu_node *old = rcu_dereference(f->published);
	bigtime_t deadline = system_time() + 100000;
	__atomic_store_n(&f->ready, 1, __ATOMIC_RELEASE);
	if (!SpinUntil(&f->nested_request, deadline)) __atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
	/* Writer is waiting for the outer section. This nested read cannot queue
	 * behind it. No allocation or blocking wait occurs while IRQs are off. */
	rcu_read_lock();
	if (!rcu_read_lock_held()) __atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
	rcu_read_unlock();
	__atomic_store_n(&f->nested_done, 1, __ATOMIC_RELEASE);
	if (!SpinUntil(&f->release_reader, deadline)) __atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
	if (!old || old->value != 915) __atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
	rcu_read_unlock();
	if (!preemptible() || rcu_read_lock_held()) __atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
	renku_linux_task_unregister(&task);
	complete(&f->done);
	return B_OK;
}
static status_t RCUWriter(void *cookie)
{
	struct rcu_fixture *f = cookie;
	struct task_struct task;
	if (!renku_linux_task_register(&task)) Fatal();
	__atomic_store_n(&f->writer_started, 1, __ATOMIC_RELEASE);
	synchronize_rcu();
	__atomic_store_n(&f->writer_done, 1, __ATOMIC_RELEASE);
	renku_linux_task_unregister(&task);
	complete(&f->done);
	return B_OK;
}
static void NativeRCUCases(struct completion_report *r)
{
	CHECK(r, !rcu_read_lock_held() && preemptible());
	rcu_read_lock(); rcu_read_lock();
	CHECK(r, rcu_read_lock_held() && preempt_count() == 2 && !interrupts_enabled());
	CHECK(r, renku_linux_rcu_backend_uninit() == B_BUSY);
	rcu_read_unlock(); CHECK(r, rcu_read_lock_held()); rcu_read_unlock();
	CHECK(r, !rcu_read_lock_held() && preemptible());
	CHECK(r, nr_cpu_ids >= 3); /* rendezvous needs reader, writer and controller */
	if (nr_cpu_ids < 3) return;
	struct rcu_node *old = kmalloc(sizeof(*old), GFP_KERNEL);
	CHECK(r, old != NULL); if (!old) return;
	old->value = 915;
	struct rcu_fixture f = {.published = old};
	init_completion(&f.done);
	/* Create both threads before a reader can enter an IRQ-off scope. */
	thread_id ids[2];
	ids[0] = spawn_kernel_thread(RCUReader, "i915 RCU reader", B_NORMAL_PRIORITY, &f);
	ids[1] = spawn_kernel_thread(RCUWriter, "i915 RCU grace", B_NORMAL_PRIORITY, &f);
	if (ids[0] < B_OK || ids[1] < B_OK) Fatal();
	if (resume_thread(ids[0]) != B_OK || !SleepUntil(&f.ready)) Fatal();
	rcu_assign_pointer(f.published, NULL);
	if (resume_thread(ids[1]) != B_OK || !SleepUntil(&f.writer_started)) Fatal();
	snooze(2000);
	CHECK(r, !__atomic_load_n(&f.writer_done, __ATOMIC_ACQUIRE));
	__atomic_store_n(&f.nested_request, 1, __ATOMIC_RELEASE);
	if (!SleepUntil(&f.nested_done)) Fatal();
	CHECK(r, !__atomic_load_n(&f.writer_done, __ATOMIC_ACQUIRE));
	__atomic_store_n(&f.release_reader, 1, __ATOMIC_RELEASE);
	if (!SleepUntil(&f.writer_done)) Fatal();
	kfree(old); /* detached object freed only after the observed grace period */
	r->rcu_frees++;
	for (unsigned i = 0; i < 2; i++) {
		if (!wait_for_completion_timeout(&f.done, 2000)) Fatal();
		Join(ids[i]);
	}
	CHECK(r, !f.errors && !rcu_access_pointer(f.published) && !Queued(&f.done));
	CHECK(r, !rcu_read_lock_held() && preempt_count() == 0 && interrupts_enabled());
	r->rcu_workers = 2;
}
#endif
