/* SPDX-License-Identifier: MIT */
/* Actual Linux ww context/initialization code; ww locking is still unported. */
#include <linux/ww_mutex.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

enum { THREADS = 8, ROUNDS = 5000 };
static DEFINE_WW_CLASS(ww_class);
static DEFINE_WD_CLASS(wd_class);
static unsigned long stamps[THREADS * ROUNDS];
static pthread_barrier_t barrier;
void panic(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	abort();
}
static void *run(void *arg)
{
	unsigned int index = *(unsigned int *)arg;
	struct task_struct task, duplicate;
	assert(renku_linux_task_register(&task));
	assert(!renku_linux_task_register(&duplicate));
	assert(current == &task);
	int b = pthread_barrier_wait(&barrier);
	assert(b == 0 || b == PTHREAD_BARRIER_SERIAL_THREAD);
	unsigned long previous = 0;
	for (unsigned int i = 0; i < ROUNDS; ++i) {
		struct ww_acquire_ctx ctx;
		ww_acquire_init(&ctx, &ww_class);
		assert(ctx.task == &task && ctx.acquired == 0 && ctx.wounded == 0);
		assert(ctx.is_wait_die == 0 && ctx.stamp > previous);
		stamps[index * ROUNDS + i] = previous = ctx.stamp;
		ww_acquire_done(&ctx);
		ww_acquire_fini(&ctx);
	}
	renku_linux_task_unregister(&task);
	return NULL;
}
static int compare(const void *a, const void *b)
{
	unsigned long aa = *(const unsigned long *)a, bb = *(const unsigned long *)b;
	return (aa > bb) - (aa < bb);
}
int main(void)
{
	ktime_t start = ktime_get();
	assert(start >= 0);
	struct task_struct task;
	assert(renku_linux_task_register(&task));
	struct ww_acquire_ctx ctx;
	ww_acquire_init(&ctx, &wd_class);
	assert(ctx.task == current && ctx.is_wait_die == 1);
	ww_acquire_fini(&ctx);
	struct ww_mutex lock;
	ww_mutex_init(&lock, &ww_class);
	assert(lock.ctx == NULL && !ww_mutex_is_locked(&lock));
	/* Test only the base mutex state; this does NOT exercise ww_mutex_lock. */
	mutex_lock(&lock.base);
	assert(ww_mutex_is_locked(&lock));
	mutex_unlock(&lock.base);
	assert(!ww_mutex_is_locked(&lock));
	ww_mutex_destroy(&lock);
	pthread_t threads[THREADS];
	unsigned int ids[THREADS];
	assert(pthread_barrier_init(&barrier, NULL, THREADS) == 0);
	for (unsigned int i = 0; i < THREADS; ++i) {
		ids[i] = i;
		assert(pthread_create(threads + i, NULL, run, ids + i) == 0);
	}
	for (unsigned int i = 0; i < THREADS; ++i) assert(pthread_join(threads[i], NULL) == 0);
	assert(pthread_barrier_destroy(&barrier) == 0);
	qsort(stamps, THREADS * ROUNDS, sizeof(stamps[0]), compare);
	for (unsigned int i = 0; i < THREADS * ROUNDS; ++i) assert(stamps[i] == i + 1);
	assert(current == &task);
	renku_linux_task_unregister(&task);
	assert(ktime_get() >= start);
	/* Re-registration must not retain a stale stack task in the registry. */
	assert(renku_linux_task_register(&task));
	renku_linux_task_unregister(&task);
	puts("CPU ww prerequisites: 40000 unique context stamps, registered task identity and mutex state; pass");
	puts("ww deadlock avoidance and native kernel execution: NOT TESTED");
	return 0;
}
