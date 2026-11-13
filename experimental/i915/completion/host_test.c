/* SPDX-License-Identifier: MIT */
#include "cases.h"
#include <stdio.h>
#include <signal.h>
static volatile sig_atomic_t received;
static void OnSignal(int signal) { (void)signal; received = 1; }
int main(void)
{
	struct completion_report report;
	RunCases(&report);
	assert(!report.failures && report.phases == 12 && report.workers == 24);
	struct sigaction handler = {.sa_handler = OnSignal};
	sigemptyset(&handler.sa_mask);
	assert(!sigaction(SIGUSR1, &handler, NULL));
	struct completion target, ready, done;
	init_completion(&target); init_completion(&ready); init_completion(&done);
	unsigned payload = 0;
	struct test_worker worker = {&target, &ready, &done, 2, &payload, 0, false};
	worker_id id = Spawn(&worker);
	/* Queue readiness alone can precede the OS block. Repeated signals cover
	 * that interval; the test has a deadline and never supplies completion. */
	assert(UntilQueued(&target, 1));
	s64 deadline = NowUS() + 1000000;
	while (Queued(&target)) {
		assert(NowUS() < deadline);
		assert(!pthread_kill(id, SIGUSR1)); PauseUS(1000);
	}
	Join(id);
	assert(worker.result == -ERESTARTSYS && received);
	FinishFixture(&target); FinishFixture(&ready); FinishFixture(&done);
	printf("PASS original completion/swait: %u checks, %u phases, %u waiters; real host signal interruption\n",
		report.checks, report.phases, report.workers);
	return 0;
}
