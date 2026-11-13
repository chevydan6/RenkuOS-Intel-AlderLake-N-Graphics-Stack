/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <native_services.h>
#include "cases.h"
extern bool interrupts_enabled(void);
static sem_id sRun = -1;
int32 api_version = B_CUR_DRIVER_API_VERSION;

struct worker_group {
	struct rw_stress stress;
	struct renku_kpi_completion ready, gate, done;
};
struct worker_arg { struct worker_group *group; bool writer; };
static void Fatal(status_t status)
{ panic("i915 VMA worker lifecycle failed: %d", status); __builtin_trap(); }
static status_t Worker(void *cookie)
{
	struct worker_arg *arg = cookie;
	struct worker_group *group = arg->group;
	if (renku_kpi_completion_signal(&group->ready) != B_OK) Fatal(B_ERROR);
	if (renku_kpi_completion_wait(&group->gate, 10000000) != B_OK) Fatal(B_TIMED_OUT);
	rw_stress_run(&group->stress, arg->writer);
	if (renku_kpi_completion_signal(&group->done) != B_OK) Fatal(B_ERROR);
	return B_OK;
}
static void Run(struct vma_native_report *out)
{
	struct vma_report *r = &out->result;
	out->version = 1;
	vma_cases(r);
	bool initial_irq = interrupts_enabled();
	rwlock_t first, second;
	rwlock_init(&first); rwlock_init(&second);
	read_lock(&first);
	VMA_CHECK(r, !interrupts_enabled());
	read_lock(&second);
	VMA_CHECK(r, !interrupts_enabled());
	read_unlock(&second);
	VMA_CHECK(r, !interrupts_enabled());
	read_unlock(&first);
	VMA_CHECK(r, interrupts_enabled() == initial_irq);
	cpu_status saved = disable_interrupts();
	write_lock(&first); write_unlock(&first);
	VMA_CHECK(r, !interrupts_enabled());
	VMA_CHECK(r, kmalloc(64, GFP_KERNEL) == NULL);
	restore_interrupts(saved);
	VMA_CHECK(r, kmalloc(64, GFP_ATOMIC) == NULL);
	void *zero = kmalloc(0, GFP_KERNEL);
	VMA_CHECK(r, zero == (void *)16); kfree(zero); kfree(NULL);
	unsigned char *bytes = kmalloc(64, GFP_KERNEL | __GFP_ZERO);
	VMA_CHECK(r, bytes != NULL);
	if (bytes) {
		for (unsigned i = 0; i < 64; i++) VMA_CHECK(r, bytes[i] == 0);
		read_lock(&first); kfree(bytes); read_unlock(&first);
	}
	struct worker_group group = {.stress = {.inverse = ~0u}};
	rwlock_init(&group.stress.lock);
	struct renku_kpi_completion *sems[] = {&group.ready, &group.gate, &group.done};
	unsigned initialized = 0;
	for (; initialized < 3; initialized++) {
		status_t status = renku_kpi_completion_init(sems[initialized]);
		VMA_CHECK(r, status == B_OK);
		if (status != B_OK) break;
	}
	if (initialized != 3) goto cleanup;
	struct worker_arg args[] = {{&group, false}, {&group, false}, {&group, true}};
	thread_id threads[3];
	unsigned spawned = 0;
	for (; spawned < 3; spawned++) {
		threads[spawned] = spawn_kernel_thread(Worker, "i915 VMA lock worker", B_NORMAL_PRIORITY, &args[spawned]);
		VMA_CHECK(r, threads[spawned] >= B_OK);
		if (threads[spawned] < B_OK) break;
		status_t status = resume_thread(threads[spawned]);
		if (status != B_OK) Fatal(status);
	}
	for (unsigned i = 0; i < spawned; i++) {
		if (renku_kpi_completion_wait(&group.ready, 10000000) != B_OK) Fatal(B_TIMED_OUT);
	}
	for (unsigned i = 0; i < spawned; i++)
		if (renku_kpi_completion_signal(&group.gate) != B_OK) Fatal(B_ERROR);
	for (unsigned i = 0; i < spawned; i++) {
		if (renku_kpi_completion_wait(&group.done, 10000000) != B_OK) Fatal(B_TIMED_OUT);
		status_t returned;
		status_t status = wait_for_thread_etc(threads[i], B_RELATIVE_TIMEOUT, 10000000, &returned);
		if (status == B_BAD_THREAD_ID) {
			thread_info info;
			if (get_thread_info(threads[i], &info) != B_BAD_THREAD_ID) Fatal(status);
		} else if (status != B_OK || returned != B_OK) Fatal(status);
	}
	out->iterations = group.stress.value * 3;
	out->max_readers = group.stress.max_readers;
	out->worker_errors = group.stress.errors;
	VMA_CHECK(r, spawned == 3);
	VMA_CHECK(r, group.stress.value == 4000 && group.stress.inverse == ~4000u);
	VMA_CHECK(r, group.stress.errors == 0 && group.stress.readers == 0);
	VMA_CHECK(r, group.stress.max_readers == 2);
cleanup:
	while (initialized) VMA_CHECK(r, renku_kpi_completion_destroy(sems[--initialized]) == B_OK);
}
static status_t Open(const char *name, uint32 flags, void **cookie)
{ (void)name; (void)flags; *cookie = NULL; return B_OK; }
static status_t Close(void *cookie) { (void)cookie; return B_OK; }
static status_t Control(void *cookie, uint32 operation, void *buffer, size_t size)
{
	(void)cookie;
	if (operation != VMA_IOCTL || size != sizeof(struct vma_native_report)) return B_BAD_VALUE;
	status_t status = acquire_sem(sRun);
	if (status != B_OK) return status;
	struct vma_native_report report = {};
	Run(&report);
	status = user_memcpy(buffer, &report, sizeof(report));
	release_sem(sRun); return status;
}
static device_hooks hooks = {Open, Close, Close, Control, NULL, NULL, NULL, NULL, NULL, NULL};
status_t init_hardware(void) { return B_OK; }
status_t init_driver(void)
{
	status_t status = renku_linux_rw_backend_init();
	if (status != B_OK) return status;
	sRun = create_sem(1, "i915 VMA tests");
	if (sRun < B_OK) { renku_linux_rw_backend_uninit(); return sRun; }
	dprintf("I915_LINUX_VMA_INIT status=0\n"); return B_OK;
}
void uninit_driver(void)
{
	status_t status = renku_linux_rw_backend_uninit();
	if (status != B_OK) Fatal(status);
	status = delete_sem(sRun);
	dprintf("I915_LINUX_VMA_UNINIT status=%d\n", status);
}
const char **publish_devices(void)
{ static const char *names[] = {"misc/i915_linux_vma_test", NULL}; return names; }
device_hooks *find_device(const char *name)
{ return strcmp(name, "misc/i915_linux_vma_test") == 0 ? &hooks : NULL; }
