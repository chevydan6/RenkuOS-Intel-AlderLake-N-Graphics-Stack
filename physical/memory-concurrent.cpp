// SPDX-License-Identifier: MIT
// Two-client allocation diagnostic; no GPU commands or buffer writes.
#include <OS.h>
#include <AGP.h>
#include <intel_extreme.h>
#include <atomic>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct Worker {
	int fd = -1;
	pthread_t thread;
	bool started = false;
	uint32 requested = 0;
	intel_allocate_graphics_memory allocation = {};
	int allocateError = 0;
	int freeError = 0;
	bool ownsBuffer = false;
	std::atomic<bool> ready{false};
	std::atomic<bool> allocated{false};
	std::atomic<bool> finished{false};
};

// Static storage and descriptors survive a stuck ioctl. Never cancel a worker.
static Worker sWorkers[2];
static std::atomic<bool> sStart{false}, sFree{false}, sCancel{false};

static bool
WaitFor(const std::atomic<bool>& flag)
{
	bigtime_t deadline = system_time() + 5000000;
	while (!flag.load()) {
		if (sCancel.load() || system_time() >= deadline)
			return false;
		snooze(1000);
	}
	return !sCancel.load();
}

static void*
Run(void* argument)
{
	Worker& worker = *static_cast<Worker*>(argument);
	worker.ready.store(true);
	if (WaitFor(sStart)) {
		if (ioctl(worker.fd, INTEL_ALLOCATE_GRAPHICS_MEMORY,
				&worker.allocation, sizeof(worker.allocation)) < 0)
			worker.allocateError = errno;
		else
			worker.ownsBuffer = true;
		worker.allocated.store(true);
		if (WaitFor(sFree) && worker.ownsBuffer) {
			intel_free_graphics_memory release = {};
			release.magic = INTEL_PRIVATE_DATA_MAGIC;
			release.buffer_base = worker.allocation.buffer_base;
			if (ioctl(worker.fd, INTEL_FREE_GRAPHICS_MEMORY,
					&release, sizeof(release)) < 0)
				worker.freeError = errno;
			else
				worker.ownsBuffer = false;
		}
	}
	worker.finished.store(true);
	return nullptr;
}

static bool
ValidRange(uintptr_t base, uint32 size, uint32 requested)
{
	return requested != 0 && size == requested && base % B_PAGE_SIZE == 0
		&& base <= UINTPTR_MAX - size;
}

static bool
SeparateRanges(uintptr_t a, uint32 aSize, uintptr_t b, uint32 bSize)
{
	return ValidRange(a, aSize, aSize) && ValidRange(b, bSize, bSize)
		&& (a + aSize <= b || b + bSize <= a);
}

static void
Quarantine()
{
	puts("INCONCLUSIVE: retaining process, descriptors and buffers; collect logs and reboot.");
	// A user deadline cannot stop a kernel ioctl. The host observer must stop
	// the experiment at 60 seconds and arrange a reset. No cleanup is attempted.
	for (;;)
		snooze(1000000);
}

static void
JoinWorkers()
{
	bigtime_t deadline = system_time() + 5000000;
	for (Worker& worker : sWorkers) {
		if (!worker.started)
			continue;
		while (!worker.finished.load()) {
			if (system_time() >= deadline)
				Quarantine();
			snooze(1000);
		}
		if (pthread_join(worker.thread, nullptr) != 0)
			Quarantine();
		worker.started = false;
	}
}

int
main(int argc, char** argv)
{
	setvbuf(stdout, nullptr, _IOLBF, 0);
	if (argc == 2 && strcmp(argv[1], "--help") == 0) {
		puts("memory-concurrent --self-test | --run /dev/graphics/DEVICE");
		puts("10 rounds, two clients, at most 128 KiB live. Keep app_server running.");
		puts("Requires a 60-second host observer; stalled/ambiguous cases retain state for reset.");
		puts("No per-fd kernel ownership proof, buffer writes or GPU execution.");
		return 0;
	}
	if (argc == 2 && strcmp(argv[1], "--self-test") == 0) {
		bool pass = SeparateRanges(4096, 4096, 8192, 8192)
			&& SeparateRanges(8192, 8192, 4096, 4096)
			&& !SeparateRanges(4096, 4096, 4096, 4096)
			&& !SeparateRanges(4096, 8192, 8192, 4096)
			&& !SeparateRanges(UINTPTR_MAX - 4095, 4096, 4096, 4096)
			&& !ValidRange(4097, 4096, 4096)
			&& !ValidRange(4096, 8192, 4096)
			&& !ValidRange(4096, 0, 0);
		puts(pass ? "8 range controls passed; no device opened." : "Range controls FAILED.");
		return pass ? 0 : 1;
	}
	if (argc != 3 || strcmp(argv[1], "--run") != 0
		|| strncmp(argv[2], "/dev/graphics/", 14) != 0)
		return 2;
	for (Worker& worker : sWorkers) {
		worker.fd = open(argv[2], O_RDWR);
		intel_get_shared_info_layout layout = {};
		layout.magic = INTEL_PRIVATE_DATA_MAGIC;
		if (worker.fd < 0 || ioctl(worker.fd, INTEL_GET_SHARED_INFO_LAYOUT,
				&layout, sizeof(layout)) < 0
			|| layout.version != INTEL_SHARED_INFO_LAYOUT_VERSION
			|| layout.shared_info_size != sizeof(intel_shared_info)) {
			puts("Open/interface check failed; no allocations attempted.");
			for (Worker& opened : sWorkers) {
				if (opened.fd >= 0)
					close(opened.fd);
			}
			return 1;
		}
	}
	const uint32 sizes[] = {4096, 8192, 65536};
	for (unsigned round = 0; round < 10; round++) {
		sStart.store(false);
		sFree.store(false);
		bool valid = true;
		for (unsigned index = 0; index < 2; index++) {
			Worker& worker = sWorkers[index];
			worker.requested = sizes[(round + index) % 3];
			worker.allocation = {};
			worker.allocation.magic = INTEL_PRIVATE_DATA_MAGIC;
			worker.allocation.size = worker.requested;
			worker.allocation.alignment = B_PAGE_SIZE;
			worker.allocation.flags = B_APERTURE_NON_RESERVED;
			worker.allocateError = worker.freeError = 0;
			worker.ready.store(false);
			worker.allocated.store(false);
			worker.finished.store(false);
			int error = pthread_create(&worker.thread, nullptr, Run, &worker);
			if (error != 0) {
				printf("pthread_create failed: %d\n", error);
				valid = false;
				break;
			}
			worker.started = true;
		}
		for (Worker& worker : sWorkers)
			valid = valid && WaitFor(worker.ready);
		if (valid) {
			sStart.store(true);
			for (Worker& worker : sWorkers)
				valid = valid && WaitFor(worker.allocated);
		}
		if (valid) {
			for (unsigned index = 0; index < 2; index++) {
				Worker& worker = sWorkers[index];
				printf("round=%u client=%u base=%#" B_PRIxADDR
					" size=%" B_PRIu32 " errno=%d\n", round, index,
					worker.allocation.buffer_base, worker.allocation.size,
					worker.allocateError);
				valid = valid && worker.allocateError == 0
					&& ValidRange(worker.allocation.buffer_base,
						worker.allocation.size, worker.requested);
			}
			valid = valid && SeparateRanges(sWorkers[0].allocation.buffer_base,
				sWorkers[0].requested, sWorkers[1].allocation.buffer_base,
				sWorkers[1].requested);
		}
		if (!valid) {
			sCancel.store(true);
			JoinWorkers();
			// Workers are joined: these fields cannot race an in-flight ioctl.
			for (unsigned index = 0; index < 2; index++) {
				Worker& worker = sWorkers[index];
				printf("Stopped client=%u allocated=%d base=%#" B_PRIxADDR
					" size=%" B_PRIu32 " allocation_errno=%d owned=%d\n",
					index, int(worker.allocated.load()),
					worker.allocation.buffer_base, worker.allocation.size,
					worker.allocateError, int(worker.ownsBuffer));
			}
			Quarantine();
		}
		// Both published allocations remain live until validation completes.
		sFree.store(true);
		JoinWorkers();
		for (Worker& worker : sWorkers) {
			if (worker.freeError != 0 || worker.ownsBuffer) {
				printf("Free incomplete: errno=%d\n", worker.freeError);
				Quarantine();
			}
		}
		printf("round=%u two distinct live ranges freed successfully\n", round);
	}
	for (Worker& worker : sWorkers)
		close(worker.fd);
	puts("10 concurrent-client rounds passed; not GPU execution proof.");
	return 0;
}
