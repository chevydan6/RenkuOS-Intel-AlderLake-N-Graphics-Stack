/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "test.h"
static volatile sig_atomic_t received;
static void OnSignal(int signal) { (void)signal; received = 1; }
struct waiter { int fd, failed, finished; unsigned operation; struct completion_signal_report report; };
static void *Wait(void *cookie)
{
	struct waiter *waiter = cookie;
	if (ioctl(waiter->fd, waiter->operation, &waiter->report, sizeof(waiter->report)) < 0) waiter->failed = 1;
	__atomic_store_n(&waiter->finished, 1, __ATOMIC_RELEASE);
	return NULL;
}
static int SignalTest(int fd, unsigned operation)
{
	struct waiter waiter = {.fd = fd, .operation = operation};
	pthread_t thread;
	received = 0;
	if (pthread_create(&thread, NULL, Wait, &waiter)) return 1;
	int queued = 0, failed = 0;
	for (unsigned i = 0; i < 1000; i++) {
		struct completion_signal_report report = {};
		if (ioctl(fd, COMPLETION_QUEUED, &report, sizeof(report)) < 0) { failed = 1; break; }
		if (report.queued == 1) { queued = 1; break; }
		if (__atomic_load_n(&waiter.finished, __ATOMIC_ACQUIRE)) break;
		usleep(1000);
	}
	if (!queued || pthread_kill(thread, SIGUSR1)) failed = 1;
	if (operation == COMPLETION_KILLABLE || failed) {
		usleep(50000);
		if (operation == COMPLETION_KILLABLE && __atomic_load_n(&waiter.finished, __ATOMIC_ACQUIRE)) failed = 1;
		struct completion_signal_report report = {};
		if (ioctl(fd, COMPLETION_SIGNAL, &report, sizeof(report)) < 0) failed = 1;
	}
	if (pthread_join(thread, NULL)) return 1;
	if (waiter.failed || !received || waiter.report.queued) failed = 1;
	if (operation == COMPLETION_INTERRUPTIBLE && waiter.report.result != -512) failed = 1;
	if (operation == COMPLETION_KILLABLE && (waiter.report.result <= 0 || waiter.report.result > 2000)) failed = 1;
	printf("mode=%s result=%ld signal_seen=%d queued_after=%u failed=%d\n",
		operation == COMPLETION_INTERRUPTIBLE ? "interruptible" : "killable-ordinary-signal",
		waiter.report.result, (int)received, waiter.report.queued, failed);
	return failed;
}
int main(void)
{
	alarm(60);
	struct sigaction handler = {.sa_handler = OnSignal};
	sigemptyset(&handler.sa_mask);
	if (sigaction(SIGUSR1, &handler, NULL)) return 1;
	int fd = open("/dev/misc/i915_linux_completion_test", O_RDONLY);
	if (fd < 0) { perror("open"); return 1; }
	puts("evidence=ORIGINAL_COMPLETION_SWAIT_NATIVE_CPU gpu_submission=false");
	int failed = 0;
	for (unsigned i = 0; i < 4; i++) {
		struct completion_report report = {};
		if (ioctl(fd, COMPLETION_RUN, &report, sizeof(report)) < 0) { perror("ioctl"); failed = 1; break; }
		printf("run=%u checks=%u failures=%u first_line=%u phases=%u waiters=%u\n",
			i, report.checks, report.failures, report.first_line, report.phases, report.workers);
		fflush(stdout);
		printf("cpu_count=%u online=%u online_mask=%#lx\n", report.cpu_count, report.online_count, report.online_bits);
		printf("bit_workers=%u bit_iterations=%u\n", report.bit_workers, report.bit_iterations);
		printf("rcu_workers=%u rcu_frees=%u\n", report.rcu_workers, report.rcu_frees);
		printf("lockref_workers=%u lockref_iterations=%u\n", report.lockref_workers, report.lockref_iterations);
		if (report.version != 5 || report.failures || report.checks < 100 || !report.cpu_count || !report.online_count || report.online_count > report.cpu_count || report.bit_workers != 2 || report.bit_iterations != 8000 || report.rcu_workers != 2 || report.rcu_frees != 1 || report.lockref_workers != 2 || report.lockref_iterations != 8000 || report.phases != 12 || report.workers != 24) { failed = 1; break; }
	}
	if (!failed) failed |= SignalTest(fd, COMPLETION_INTERRUPTIBLE);
	if (!failed) failed |= SignalTest(fd, COMPLETION_KILLABLE);
	close(fd);
	return failed;
}
