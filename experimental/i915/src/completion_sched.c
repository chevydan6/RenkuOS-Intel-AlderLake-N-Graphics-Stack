/* SPDX-License-Identifier: MIT */
/* Event/deadline adapter for original Linux completion.c and swait.c.
 * Registered scopes are not Linux scheduler tasks or general task references. */
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/jiffies.h>
#ifndef RENKU_NATIVE
#include <errno.h>
#include <time.h>
#endif
#ifdef RENKU_NATIVE
extern bool interrupts_enabled(void);
#endif

static void Failed(const char *operation)
{ panic("i915 wait adapter: %s", operation); __builtin_trap(); }
static s64 ClockUS(void)
{
#ifdef RENKU_NATIVE
	return system_time();
#else
	return ktime_get() / 1000;
#endif
}

static void ReleaseWait(struct task_struct *task)
{
	if (READ_ONCE(task->wait_state) != TASK_RUNNING) Failed("task still waiting at scope exit");
#ifdef RENKU_NATIVE
	if (delete_sem(task->wait_event) != B_OK) Failed("delete wait semaphore");
#else
	if (sem_destroy(&task->wait_event) != 0) Failed("destroy host wait semaphore");
#endif
	task->wait_initialized = false;
	task->release_wait = NULL;
}

void renku_linux_might_sleep(void)
{
#ifdef RENKU_NATIVE
	if (!interrupts_enabled()) Failed("sleep with interrupts disabled");
#endif
	struct task_struct *task = current;
	if (task->wait_initialized) {
		/* A wake between state publication and OS blocking can leave one
		 * semaphore credit after the original algorithm observes ->done.
		 * Before this scope publishes another wait, discard stale credits. */
		if (READ_ONCE(task->wait_state) != TASK_RUNNING) Failed("nested wait publication");
		for (;;) {
#ifdef RENKU_NATIVE
			status_t status = acquire_sem_etc(task->wait_event, 1, B_RELATIVE_TIMEOUT, 0);
			if (status == B_OK) continue;
			if (status != B_WOULD_BLOCK && status != B_TIMED_OUT) Failed("drain stale wait event");
#else
			if (sem_trywait(&task->wait_event) == 0) continue;
			if (errno == EINTR) continue;
			if (errno != EAGAIN) Failed("drain stale host wait event");
#endif
			break;
		}
		return;
	}
#ifdef RENKU_NATIVE
	task->wait_event = create_sem(0, "i915 Linux wait event");
	if (task->wait_event < B_OK) Failed("create wait semaphore");
#else
	if (sem_init(&task->wait_event, 0, 0) != 0) Failed("initialize host wait semaphore");
#endif
	task->release_wait = ReleaseWait;
	task->wait_initialized = true;
}

void renku_linux_set_current_state(unsigned int state)
{
	struct task_struct *task = current;
	if (!task->wait_initialized) Failed("wait event not prepared before queue publication");
	if (state != TASK_RUNNING && state != TASK_UNINTERRUPTIBLE
		&& state != TASK_INTERRUPTIBLE && state != TASK_KILLABLE) Failed("unsupported task wait state");
	__atomic_store_n(&task->wait_state, state, __ATOMIC_SEQ_CST);
}

int try_to_wake_up(struct task_struct *task, unsigned int mask, int flags)
{
	(void)flags; /* CPU-placement hints do not change native completion semantics. */
	if (!task->wait_initialized) Failed("wake of unprepared task");
	unsigned int state = READ_ONCE(task->wait_state);
	while (state & mask) {
		if (!__atomic_compare_exchange_n(&task->wait_state, &state, TASK_RUNNING,
			false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) continue;
#ifdef RENKU_NATIVE
		if (release_sem_etc(task->wait_event, 1, B_DO_NOT_RESCHEDULE) != B_OK) Failed("signal wait semaphore");
#else
		if (sem_post(&task->wait_event) != 0) Failed("signal host wait semaphore");
#endif
		return 1;
	}
	return 0;
}
int wake_up_state(struct task_struct *task, unsigned int mask)
{ return try_to_wake_up(task, mask, 0); }

int renku_linux_signal_pending_state(unsigned int state, struct task_struct *task)
{
	/* Native acquisition checks pending signals before sleeping. An interrupted
	 * wait is then reported to the original completion loop, in this scope and
	 * for the wait mode that requested it. Uninterruptible waits ignore signals. */
	return state != TASK_UNINTERRUPTIBLE && task->interrupted_state == state;
}

long schedule_timeout(long ticks)
{
	struct task_struct *task = current;
	if (!task->wait_initialized) Failed("schedule without prepared event");
	unsigned int state = READ_ONCE(task->wait_state);
	if (ticks <= 0 || state == TASK_RUNNING) {
		WRITE_ONCE(task->wait_state, TASK_RUNNING);
		return ticks > 0 ? ticks : 0;
	}
	const bool infinite = ticks == MAX_SCHEDULE_TIMEOUT;
	s64 now = ClockUS();
	s64 duration = ticks > (INT64_MAX - now) / 1000 ? INT64_MAX - now : ticks * 1000;
	s64 deadline = now + duration;
	bool interrupted = false;
	for (;;) {
#ifdef RENKU_NATIVE
		uint32 flags = B_ABSOLUTE_TIMEOUT;
		if (state == TASK_INTERRUPTIBLE) flags |= B_CAN_INTERRUPT;
		else if (state == TASK_KILLABLE) flags |= B_KILL_CAN_INTERRUPT;
		status_t status = acquire_sem_etc(task->wait_event, 1, flags,
			infinite ? B_INFINITE_TIMEOUT : deadline);
		if (status == B_INTERRUPTED && state == TASK_UNINTERRUPTIBLE) continue;
		if (status == B_INTERRUPTED) interrupted = true;
		else if (status != B_OK && status != B_TIMED_OUT && status != B_WOULD_BLOCK)
			Failed("wait semaphore acquisition");
#else
		int result;
		if (infinite) result = sem_wait(&task->wait_event);
		else {
			struct timespec limit = {.tv_sec = deadline / 1000000,
				.tv_nsec = (deadline % 1000000) * 1000};
			result = sem_clockwait(&task->wait_event, CLOCK_MONOTONIC, &limit);
		}
		if (result != 0 && errno == EINTR) {
			/* POSIX has no kill-only semaphore wait; host killable waits
			 * remain uninterruptible. Native uses B_KILL_CAN_INTERRUPT. */
			if (state != TASK_INTERRUPTIBLE) continue;
			interrupted = true;
		} else if (result != 0 && errno != ETIMEDOUT) Failed("host wait acquisition");
#endif
		break;
	}
	task->interrupted_state = interrupted ? state : 0;
	WRITE_ONCE(task->wait_state, TASK_RUNNING);
	if (infinite) return ticks;
	s64 remaining = deadline - ClockUS();
	return remaining > 0 ? min(ticks, (long)(remaining / 1000 + !!(remaining % 1000))) : 0;
}
long io_schedule_timeout(long ticks)
{ return schedule_timeout(ticks); /* Native has no Linux blkio wait accounting. */ }

/* Header users may reach the sleep annotation before linux/kernel.h defines
 * its macro. Keep that declaration connected to the same native check. */
void might_sleep(void) { renku_linux_might_sleep(); }
