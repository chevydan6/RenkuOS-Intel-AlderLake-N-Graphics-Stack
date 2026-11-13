/* SPDX-License-Identifier: MIT */
/* CPU-only kernel integration driver. It never claims or accesses a GPU. */
#include <KernelExport.h>
#include <Drivers.h>
#include <unistd.h>
#include "native_test_ioctl.h"
#include <native_services.h>
#include <native_irq_scopes.h>
#include <linux/kref.h>
#include <linux/mutex.h>
#include <linux/ww_mutex.h>
#include <drm/drm_mm.h>

int32 api_version = B_CUR_DRIVER_API_VERSION;
static const char *devices[] = { KPI_TEST_DEVICE, NULL };
static sem_id run_lock = -1;
#define CHECK(r, x) do { (r)->checks++; if (!(x)) { (r)->failures++; \
	if (!(r)->first_failure_line) (r)->first_failure_line = __LINE__; } } while (0)

enum { WORKERS = 2, ITERATIONS = 2000 };
struct worker_shared {
	struct mutex mutex;
	spinlock_t spin;
	struct renku_kpi_completion ready, gate, done;
	struct kref ref;
	struct ww_class ww;
	uint32 mutex_count, spin_count, releases;
	int32 errors;
};
static void release_ref(struct kref *ref)
{
	struct worker_shared *s = container_of(ref, struct worker_shared, ref);
	s->releases++;
}
static status_t worker(void *data)
{
	struct worker_shared *s = data;
	struct task_struct task;
	bool registered = renku_linux_task_register(&task);
	renku_kpi_completion_signal(&s->ready);
	if (!registered || renku_kpi_completion_wait(&s->gate, 10000000) != B_OK) {
		__atomic_fetch_add(&s->errors, 1, __ATOMIC_RELAXED);
	} else {
		unsigned long previous = 0;
		for (unsigned int i = 0; i < ITERATIONS; ++i) {
			struct ww_acquire_ctx ctx;
			ww_acquire_init(&ctx, &s->ww);
			if (ctx.task != &task || ctx.stamp <= previous)
				__atomic_fetch_add(&s->errors, 1, __ATOMIC_RELAXED);
			previous = ctx.stamp;
			ww_acquire_fini(&ctx);
			mutex_lock(&s->mutex);
			s->mutex_count++;
			mutex_unlock(&s->mutex);
			spin_lock(&s->spin);
			s->spin_count++;
			spin_unlock(&s->spin);
			kref_get(&s->ref);
			if (kref_put(&s->ref, release_ref))
				__atomic_fetch_add(&s->errors, 1, __ATOMIC_RELAXED);
		}
	}
	if (registered) renku_linux_task_unregister(&task);
	renku_kpi_completion_signal(&s->done);
	return B_OK;
}
static void fatal_worker(status_t error)
{
	/* A live worker cannot outlive this ioctl's stack or its loaded code. */
	panic("i915 native test: worker failed to finish: %d", error);
	__builtin_trap();
}
static void test_threads(struct kpi_test_report *r)
{
	struct worker_shared s = { .ww = __WW_CLASS_INITIALIZER(native_test, 0) };
	struct renku_kpi_completion *completions[] = { &s.ready, &s.gate, &s.done };
	unsigned int initialized = 0;
	for (; initialized < 3; ++initialized) {
		status_t error = renku_kpi_completion_init(completions[initialized]);
		CHECK(r, error == B_OK);
		if (error != B_OK) break;
	}
	if (initialized != 3) goto cleanup;
	mutex_init(&s.mutex);
	spin_lock_init(&s.spin);
	kref_init(&s.ref);
	thread_id threads[WORKERS];
	unsigned int spawned = 0;
	for (; spawned < WORKERS; ++spawned) {
		threads[spawned] = spawn_kernel_thread(worker, "i915 kpi CPU worker", B_NORMAL_PRIORITY, &s);
		CHECK(r, threads[spawned] >= 0);
		if (threads[spawned] < 0) break;
		status_t error = resume_thread(threads[spawned]);
		if (error != B_OK) fatal_worker(error);
	}
	r->kernel_threads = spawned;
	for (unsigned int i = 0; i < spawned; ++i) {
		status_t error = renku_kpi_completion_wait(&s.ready, 10000000);
		if (error != B_OK) fatal_worker(error);
	}
	for (unsigned int i = 0; i < spawned; ++i)
		CHECK(r, renku_kpi_completion_signal(&s.gate) == B_OK);
	for (unsigned int i = 0; i < spawned; ++i) {
		status_t error = renku_kpi_completion_wait(&s.done, 10000000);
		if (error != B_OK) fatal_worker(error);
		status_t returned;
		error = wait_for_thread_etc(threads[i], B_RELATIVE_TIMEOUT, 10000000, &returned);
		if (error == B_BAD_THREAD_ID) {
			/* RenkuOS thread_exit hides kernel threads after their entry
			 * function returns, without caching a death entry. The done
			 * notification above and absence from the native thread table
			 * establish that the callback can no longer use this stack.
			 * No native return code is available for this late-join case. */
			thread_info info;
			error = get_thread_info(threads[i], &info);
			if (error != B_BAD_THREAD_ID) fatal_worker(error);
			CHECK(r, error == B_BAD_THREAD_ID);
		} else {
			if (error != B_OK) fatal_worker(error);
			CHECK(r, returned == B_OK);
		}
	}
	CHECK(r, s.errors == 0);
	CHECK(r, s.mutex_count == WORKERS * ITERATIONS);
	CHECK(r, s.spin_count == WORKERS * ITERATIONS);
	CHECK(r, READ_ONCE(s.ww.stamp.counter) == WORKERS * ITERATIONS);
	CHECK(r, kref_read(&s.ref) == 1 && s.releases == 0);
	CHECK(r, kref_put(&s.ref, release_ref) == 1 && s.releases == 1);
	r->worker_iterations = s.mutex_count;
	r->final_releases = s.releases;
	mutex_destroy(&s.mutex);
cleanup:
	while (initialized) CHECK(r, renku_kpi_completion_destroy(completions[--initialized]) == B_OK);
}
static void run_tests(struct kpi_test_report *r)
{
	bigtime_t start = system_time();
	*r = (struct kpi_test_report){ .abi_version = 1 };
	struct renku_kpi_page pages[4];
	for (unsigned int i = 0; i < 4; ++i) {
		status_t error = renku_kpi_page_create(pages + i);
		CHECK(r, error == B_OK);
		if (pages[i].area >= 0) r->allocated_pages++;
		if (error == B_OK) {
			CHECK(r, pages[i].physical_valid && (pages[i].physical & 4095) == 0);
			CHECK(r, ((uintptr_t)pages[i].cpu & 4095) == 0);
			for (unsigned int j = 0; j < 1024; ++j) ((uint32 *)pages[i].cpu)[j] = (i << 16) | j;
		}
	}
	for (unsigned int i = 0; i < 4; ++i) {
		if (pages[i].area < 0) continue;
		if (pages[i].physical_valid)
			for (unsigned int j = 0; j < 1024; ++j)
				if (((uint32 *)pages[i].cpu)[j] != ((i << 16) | j)) { CHECK(r, false); break; }
		area_id area = pages[i].area;
		status_t error = renku_kpi_page_destroy(pages + i);
		CHECK(r, error == B_OK);
		if (error != B_OK) {
			panic("i915 native test: wired backing cleanup failed: %d", error);
			__builtin_trap();
		}
		if (error == B_OK) r->freed_pages++;
		CHECK(r, pages[i].area == -1 && !pages[i].physical_valid);
		CHECK(r, renku_kpi_page_destroy(pages + i) == B_BAD_VALUE);
		area_info info;
		/* AddressSpaceReadLocker::SetFromArea returns B_BAD_TEAM_ID when
		 * the deleted area's owning address space cannot be looked up. */
		CHECK(r, get_area_info(area, &info) == B_BAD_TEAM_ID);
	}
	struct drm_mm mm;
	struct drm_mm_node node = {0}, overlap = {0};
	drm_mm_init(&mm, 0x100000000ULL, 0x10000);
	CHECK(r, drm_mm_insert_node(&mm, &node, 4096) == 0);
	CHECK(r, node.start == 0x100000000ULL);
	overlap.start = node.start; overlap.size = 4096;
	CHECK(r, drm_mm_reserve_node(&mm, &overlap) == -ENOSPC);
	if (drm_mm_node_allocated(&node)) drm_mm_remove_node(&node);
	CHECK(r, drm_mm_clean(&mm));
	drm_mm_takedown(&mm);
	struct renku_kpi_completion c;
	status_t error = renku_kpi_completion_init(&c);
	CHECK(r, error == B_OK);
	if (error == B_OK) {
		CHECK(r, renku_kpi_completion_wait(&c, -1) == B_BAD_VALUE);
		CHECK(r, renku_kpi_completion_wait(&c, INT64_MAX) == B_BAD_VALUE);
		bigtime_t before = system_time();
		r->timeout_status = renku_kpi_completion_wait(&c, 25000);
		r->timeout_elapsed_us = system_time() - before;
		CHECK(r, r->timeout_status == B_TIMED_OUT);
		CHECK(r, r->timeout_elapsed_us >= 20000);
		CHECK(r, renku_kpi_completion_signal(&c) == B_OK);
		CHECK(r, renku_kpi_completion_wait(&c, 1000000) == B_OK);
		sem_id id = c.semaphore;
		CHECK(r, renku_kpi_completion_destroy(&c) == B_OK);
		sem_info info;
		CHECK(r, get_sem_info(id, &info) == B_BAD_SEM_ID);
		CHECK(r, renku_kpi_completion_signal(&c) == B_BAD_SEM_ID);
		CHECK(r, renku_kpi_completion_wait(&c, 1000) == B_BAD_SEM_ID);
		CHECK(r, renku_kpi_completion_destroy(&c) == B_BAD_SEM_ID);
	}
	test_threads(r);
	r->elapsed_us = system_time() - start;
}
static status_t open_device(const char *name, uint32 flags, void **cookie)
{
	(void)name; (void)flags;
	if (geteuid() != 0) return B_NOT_ALLOWED;
	*cookie = NULL;
	return B_OK;
}
static status_t close_device(void *cookie) { (void)cookie; return B_OK; }
static status_t control(void *cookie, uint32 operation, void *buffer, size_t size)
{
	(void)cookie;
	if (geteuid() != 0) return B_NOT_ALLOWED;
	if (operation != KPI_RUN_SELFTEST) return B_DEV_INVALID_IOCTL;
	if (size != sizeof(struct kpi_test_report)) return B_BAD_VALUE;
	struct kpi_test_report report = {0};
	status_t error = user_memcpy(buffer, &report, sizeof(report));
	if (error != B_OK) return error;
	do { error = acquire_sem(run_lock); } while (error == B_INTERRUPTED);
	if (error != B_OK) return error;
	run_tests(&report);
	error = release_sem(run_lock);
	if (error != B_OK) return error;
	return user_memcpy(buffer, &report, sizeof(report));
}
static status_t read_device(void *cookie, off_t pos, void *buffer, size_t *size)
{ (void)cookie; (void)pos; (void)buffer; *size = 0; return B_NOT_ALLOWED; }
static status_t write_device(void *cookie, off_t pos, const void *buffer, size_t *size)
{ (void)cookie; (void)pos; (void)buffer; *size = 0; return B_NOT_ALLOWED; }
static device_hooks hooks = { open_device, close_device, close_device, control,
	read_device, write_device, NULL, NULL, NULL, NULL };
status_t init_hardware(void) { return B_OK; }
status_t init_driver(void)
{
	status_t status = renku_linux_irq_backend_init();
	if (status != B_OK) return status;
	run_lock = create_sem(1, "i915 kpi test run");
	if (run_lock < B_OK) renku_linux_irq_backend_uninit();
	dprintf("I915_LINUXKPI_TEST_INIT status=%d\n", run_lock < 0 ? run_lock : B_OK);
	return run_lock < 0 ? run_lock : B_OK;
}
void uninit_driver(void)
{
	status_t error = run_lock >= 0 ? delete_sem(run_lock) : B_OK;
	if (error == B_OK) error = renku_linux_irq_backend_uninit();
	dprintf("I915_LINUXKPI_TEST_UNINIT status=%d\n", error);
}
const char **publish_devices(void) { return devices; }
device_hooks *find_device(const char *name) { return strcmp(name, KPI_TEST_DEVICE) == 0 ? &hooks : NULL; }
