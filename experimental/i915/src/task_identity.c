/* SPDX-License-Identifier: MIT */
#include <linux/sched.h>
#include <linux/spinlock.h>

static struct task_struct *tasks;
#ifdef RENKU_NATIVE
static spinlock_t registry_lock = { .native = B_SPINLOCK_INITIALIZER };
typedef thread_id identity_t;
static identity_t caller_id(void) { return find_thread(NULL); }
static bool same_id(identity_t a, identity_t b) { return a == b; }
#else
static spinlock_t registry_lock = { .native = PTHREAD_MUTEX_INITIALIZER };
typedef pthread_t identity_t;
static identity_t caller_id(void) { return pthread_self(); }
static bool same_id(identity_t a, identity_t b) { return pthread_equal(a, b); }
#endif

/* The caller owns storage and must retain it until all contexts using this
 * task have finished. Registration does not provide scheduler/task references. */
bool renku_linux_task_register(struct task_struct *task)
{
	identity_t id = caller_id();
	spin_lock(&registry_lock);
	for (struct task_struct *it = tasks; it; it = it->registry_next) {
		if (it == task || same_id(it->native_id, id)) {
			spin_unlock(&registry_lock);
			return false;
		}
	}
	task->native_id = id;
	task->wait_initialized = false;
	task->wait_state = TASK_RUNNING;
	task->interrupted_state = 0;
	task->release_wait = NULL;
	task->registry_next = tasks;
	tasks = task;
	spin_unlock(&registry_lock);
	return true;
}

struct task_struct *renku_linux_current(void)
{
	identity_t id = caller_id();
	struct task_struct *found = NULL;
	spin_lock(&registry_lock);
	for (struct task_struct *it = tasks; it; it = it->registry_next) {
		if (same_id(it->native_id, id)) { found = it; break; }
	}
	spin_unlock(&registry_lock);
	if (!found) {
		panic("i915 LinuxKPI: current outside registered task scope");
		__builtin_trap();
	}
	return found;
}

void renku_linux_task_unregister(struct task_struct *task)
{
	identity_t id = caller_id();
	spin_lock(&registry_lock);
	struct task_struct **slot = &tasks;
	while (*slot && *slot != task) slot = &(*slot)->registry_next;
	if (!*slot || !same_id(task->native_id, id)) {
		spin_unlock(&registry_lock);
		panic("i915 LinuxKPI: invalid task unregister");
		__builtin_trap();
	}
	*slot = task->registry_next;
	task->registry_next = NULL;
	spin_unlock(&registry_lock);
	/* No queued waiter may outlive its owning registered scope. Event cleanup
	 * runs outside the registry spinlock because native destruction may sleep.
	 * Older non-waiting scopes own no event. */
	if (task->release_wait != NULL) task->release_wait(task);
}

bool renku_linux_tasks_empty(void)
{
	spin_lock(&registry_lock);
	bool empty = tasks == NULL;
	spin_unlock(&registry_lock);
	return empty;
}
