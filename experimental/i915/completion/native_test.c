/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include "cases.h"
#include <native_irq_scopes.h>
#include <linux/rcupdate.h>
int32 api_version = B_CUR_DRIVER_API_VERSION;
static sem_id sRun = -1;
static DECLARE_COMPLETION(sSignal);
static status_t Open(const char *name, uint32 flags, void **cookie)
{ (void)name; (void)flags; *cookie = NULL; return B_OK; }
static status_t Close(void *cookie) { (void)cookie; return B_OK; }
static status_t Control(void *cookie, uint32 operation, void *buffer, size_t size)
{
	(void)cookie;
	if (operation == COMPLETION_QUEUED || operation == COMPLETION_SIGNAL) {
		if (size != sizeof(struct completion_signal_report)) return B_BAD_VALUE;
		if (operation == COMPLETION_SIGNAL) complete(&sSignal);
		struct completion_signal_report out = {.queued = Queued(&sSignal)};
		return user_memcpy(buffer, &out, sizeof(out));
	}
	if ((operation == COMPLETION_RUN && size != sizeof(struct completion_report))
		|| (operation != COMPLETION_RUN && operation != COMPLETION_INTERRUPTIBLE && operation != COMPLETION_KILLABLE)
		|| (operation != COMPLETION_RUN && size != sizeof(struct completion_signal_report))) return B_BAD_VALUE;
	status_t status = acquire_sem(sRun);
	if (status != B_OK) return status;
	if (operation == COMPLETION_RUN) {
		struct completion_report report;
		RunCases(&report);
		status = user_memcpy(buffer, &report, sizeof(report));
	} else {
		struct task_struct task;
		if (!renku_linux_task_register(&task)) Fatal();
		reinit_completion(&sSignal);
		struct completion_signal_report out = {};
		out.result = operation == COMPLETION_INTERRUPTIBLE
			? wait_for_completion_interruptible_timeout(&sSignal, 2000)
			: wait_for_completion_killable_timeout(&sSignal, 2000);
		out.queued = Queued(&sSignal);
		renku_linux_task_unregister(&task);
		status = user_memcpy(buffer, &out, sizeof(out));
	}
	release_sem(sRun); return status;
}
static device_hooks hooks = {Open, Close, Close, Control, NULL, NULL, NULL, NULL, NULL, NULL};
status_t init_hardware(void) { return B_OK; }
status_t init_driver(void)
{
	status_t status = renku_linux_cpu_backend_init();
	if (status != B_OK) return status;
	status = renku_linux_irq_backend_init();
	if (status != B_OK) { renku_linux_cpu_backend_uninit(); return status; }
	status = renku_linux_rcu_backend_init();
	if (status != B_OK) { renku_linux_irq_backend_uninit(); renku_linux_cpu_backend_uninit(); return status; }
	if (renku_linux_pages_init() != 0) {
		renku_linux_rcu_backend_uninit(); renku_linux_irq_backend_uninit(); renku_linux_cpu_backend_uninit();
		return B_ERROR;
	}
	sRun = create_sem(1, "i915 completion tests");
	if (sRun < B_OK) { renku_linux_pages_uninit(); renku_linux_rcu_backend_uninit(); renku_linux_irq_backend_uninit(); renku_linux_cpu_backend_uninit(); return sRun; }
	dprintf("I915_LINUX_COMPLETION_INIT status=0\n"); return B_OK;
}
void uninit_driver(void)
{
	if (!renku_linux_tasks_empty() || Queued(&sSignal)) Fatal();
	if (renku_linux_pages_drain(5000) || renku_linux_pages_uninit()) Fatal();
	status_t status = delete_sem(sRun);
	if (status == B_OK) status = renku_linux_rcu_backend_uninit();
	if (status == B_OK) status = renku_linux_irq_backend_uninit();
	renku_linux_cpu_backend_uninit();
	dprintf("I915_LINUX_COMPLETION_UNINIT status=%d\n", status);
}
const char **publish_devices(void)
{ static const char *names[] = {"misc/i915_linux_completion_test", NULL}; return names; }
device_hooks *find_device(const char *name)
{ return strcmp(name, "misc/i915_linux_completion_test") == 0 ? &hooks : NULL; }
