// SPDX-License-Identifier: MIT
// Native userspace reproduction of semaphore owner exit/exec behavior.
// No driver installation, GPU access or kernel instrumentation is required.
#include <OS.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

static bool
transfer(int fd, void* data, size_t length, bool writing)
{
	char* cursor = (char*)data;
	while (length > 0) {
		ssize_t count = writing ? write(fd, cursor, length) : read(fd, cursor, length);
		if (count < 0 && errno == EINTR)
			continue;
		if (count <= 0)
			return false;
		cursor += count;
		length -= count;
	}
	return true;
}

static int32
waiter(void* data)
{
	return acquire_sem_etc(*(sem_id*)data, 1, B_RELATIVE_TIMEOUT, 2000000);
}

static bool
run_case(const char* program, bool execute)
{
	int command[2];
	int report[2];
	if (pipe(command) != 0)
		return false;
	if (pipe(report) != 0) {
		close(command[0]);
		close(command[1]);
		return false;
	}
	pid_t child = fork();
	if (child < 0) {
		close(command[0]); close(command[1]);
		close(report[0]); close(report[1]);
		return false;
	}
	if (child == 0) {
		close(command[1]);
		close(report[0]);
		sem_id sem = create_sem(0, "GPU retrace owner lifetime test");
		char action;
		if (sem < B_OK || !transfer(report[1], &sem, sizeof(sem), true)
			|| !transfer(command[0], &action, 1, false)) {
			_exit(2);
		}
		if (action == 'e') {
			char readFD[32], writeFD[32];
			snprintf(readFD, sizeof(readFD), "%d", command[0]);
			snprintf(writeFD, sizeof(writeFD), "%d", report[1]);
			if (fcntl(command[0], F_SETFD, 0) < 0
				|| fcntl(report[1], F_SETFD, 0) < 0)
				_exit(3);
			execl(program, program, "--after-exec", readFD, writeFD, (char*)NULL);
			_exit(4);
		}
		_exit(action == 'x' ? 0 : 5);
	}
	close(command[0]);
	close(report[1]);
	sem_id sem = -1;
	thread_id waiting = -1;
	bool reaped = false;
	auto fail = [&](const char* stage) {
		fprintf(stderr, "FAIL owner_%s stage=%s\n", execute ? "exec" : "exit", stage);
		if (!reaped) {
			kill(child, SIGKILL);
			int status;
			while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
		}
		if (waiting >= B_OK) {
			kill_thread(waiting);
			status_t status;
			wait_for_thread(waiting, &status);
		}
		close(command[1]);
		close(report[0]);
		return false;
	};
	if (!transfer(report[0], &sem, sizeof(sem), false))
		return fail("semaphore handshake");
	sem_info info;
	if (get_sem_info(sem, &info) != B_OK || info.team != child)
		return fail("initial owner");
	waiting = spawn_thread(waiter, "foreign semaphore waiter", B_NORMAL_PRIORITY, &sem);
	if (waiting < B_OK || resume_thread(waiting) != B_OK)
		return fail("waiter start");
	bigtime_t deadline = system_time() + 1000000;
	int32 count = 0;
	while (system_time() < deadline) {
		if (get_sem_count(sem, &count) != B_OK)
			return fail("waiter count");
		if (count == -1)
			break;
		snooze(100);
	}
	if (count != -1)
		return fail("waiter did not block");
	char action = execute ? 'e' : 'x';
	if (!transfer(command[1], &action, 1, true))
		return fail("owner action");
	int childStatus = -1;
	if (execute) {
		char ready = 0;
		if (!transfer(report[0], &ready, 1, false) || ready != 'E')
			return fail("exec handshake");
		// Sem deletion occurs during exec, while this same owner team lives.
		pid_t status = waitpid(child, &childStatus, WNOHANG);
		if (status != 0) {
			reaped = status == child;
			return fail("exec owner no longer alive");
		}
	} else {
		pid_t status = waitpid(child, &childStatus, 0);
		reaped = status == child;
		if (!reaped || !WIFEXITED(childStatus) || WEXITSTATUS(childStatus) != 0)
			return fail("owner exit");
	}
	status_t waiterStatus = B_ERROR;
	if (wait_for_thread(waiting, &waiterStatus) != B_OK)
		return fail("waiter join");
	waiting = -1;
	bool ok = true;
	status_t infoStatus = get_sem_info(sem, &info);
	status_t countStatus = get_sem_count(sem, &count);
	status_t releaseStatus = release_sem(sem);
	status_t deleteStatus = delete_sem(sem);
	sem_id replacement = create_sem(0, "replacement owner lifetime test");
	ok = waiterStatus == B_BAD_SEM_ID && infoStatus == B_BAD_SEM_ID
		&& countStatus == B_BAD_SEM_ID && releaseStatus == B_BAD_SEM_ID
		&& deleteStatus == B_BAD_SEM_ID && replacement >= B_OK
		&& replacement != sem && ok;
	if (replacement >= B_OK)
		ok = delete_sem(replacement) == B_OK && ok;
	if (execute) {
		action = 'q';
		ok = transfer(command[1], &action, 1, true) && ok;
		ok = waitpid(child, &childStatus, 0) == child
			&& WIFEXITED(childStatus) && WEXITSTATUS(childStatus) == 0 && ok;
	}
	close(command[1]);
	close(report[0]);
	printf("%s owner_%s old=%ld replacement=%ld waiter=%ld info=%ld "
		"count=%ld release=%ld delete=%ld expected_bad_id=%ld\n",
		ok ? "PASS" : "FAIL", execute ? "exec" : "exit", (long)sem,
		(long)replacement, (long)waiterStatus, (long)infoStatus, (long)countStatus,
		(long)releaseStatus, (long)deleteStatus, (long)B_BAD_SEM_ID);
	return ok;
}

int
main(int argc, char** argv)
{
	alarm(15);
	signal(SIGPIPE, SIG_IGN);
	if (argc == 4 && strcmp(argv[1], "--after-exec") == 0) {
		char ready = 'E';
		char action = 0;
		return transfer(atoi(argv[3]), &ready, 1, true)
			&& transfer(atoi(argv[2]), &action, 1, false) && action == 'q' ? 0 : 6;
	}
	bool exited = run_case(argv[0], false);
	bool executed = run_case(argv[0], true);
	printf("%s native semaphore owner lifetime; no GPU execution\n",
		exited && executed ? "PASS" : "FAIL");
	return exited && executed ? 0 : 1;
}
