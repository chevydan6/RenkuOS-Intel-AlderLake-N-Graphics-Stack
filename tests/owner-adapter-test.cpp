// SPDX-License-Identifier: MIT
// Actual adapter functions, mocked GART and kernel mutex. No hardware access.
#include "IntelApertureLeaseState.h"
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <new>
#include <thread>
using status_t = int;
using uint64 = uint64_t;
using uint32 = uint32_t;
using addr_t = uintptr_t;
using phys_addr_t = uint64_t;
#ifndef B_OK
constexpr int B_OK = 0, B_BUSY = -1, B_NO_MEMORY = -2, B_BAD_VALUE = -3;
#endif
struct aperture_info {};
using mutex = std::mutex;
static void mutex_lock(mutex* lock) { lock->lock(); }
static void mutex_unlock(mutex* lock) { lock->unlock(); }
struct agp_gart_module_info {
	status_t (*get_aperture_info)(int, aperture_info*);
	status_t (*allocate_memory)(int, size_t, size_t, uint32, addr_t*, phys_addr_t*);
	status_t (*free_memory)(int, addr_t);
};
static mutex sLock;
static IntelApertureLeaseState sState;
static agp_gart_module_info* sGART;
static int sAperture = 17;
#include "adapter-under-test.inc"

using Owner = IntelApertureLeaseState;
static std::mutex sBackendLock;
static std::condition_variable sChanged;
static bool sEntered, sReturn;
static unsigned sCalls;
static status_t sBackendResult;

static status_t
Backend(int aperture)
{
	assert(aperture == 17);
	std::unique_lock<std::mutex> lock(sBackendLock);
	sCalls++;
	sEntered = true;
	sChanged.notify_all();
	assert(sChanged.wait_for(lock, std::chrono::seconds(5), [] { return sReturn; }));
	return sBackendResult;
}

static status_t QueryBackend(int aperture, aperture_info*) { return Backend(aperture); }
static status_t
AllocateBackend(int aperture, size_t size, size_t alignment, uint32 flags,
	addr_t* base, phys_addr_t* physical)
{
	assert(size == 8192 && alignment == 4096 && flags == 7);
	status_t result = Backend(aperture);
	if (result == B_OK) {
		*base = 0x10000;
		*physical = 0x20000;
	}
	return result;
}
static status_t
FreeBackend(int aperture, addr_t base)
{
	assert(base == 0x10000);
	return Backend(aperture);
}

static void
RunCase(unsigned operation, bool failed, bool initializing)
{
	// Previous workers are joined before resetting the retained model record.
	sState.~IntelApertureLeaseState();
	new (&sState) Owner();
	sEntered = sReturn = false;
	sCalls = 0;
	sBackendResult = failed ? -99 : B_OK;
	uint64 lease = 0;
	assert(sState.BeginInitialization(lease) == Owner::Success);
	if (!initializing)
		assert(FinishDisplay(lease, true) == B_OK);
	status_t result = 42;
	addr_t base = 0;
	phys_addr_t physical = 0;
	std::thread worker([&] {
		aperture_info info;
		if (operation == 0)
			result = Query(lease, &info);
		else if (operation == 1)
			result = Allocate(lease, 8192, 4096, 7, &base, &physical);
		else
			result = Free(lease, 0x10000);
	});
	{
		std::unique_lock<std::mutex> lock(sBackendLock);
		assert(sChanged.wait_for(lock, std::chrono::seconds(5), [] { return sEntered; }));
	}
	// Release must complete while the backend is still gated. Holding the owner
	// lock across GART would deadlock here and fail the external test timeout.
	assert(ReleaseDisplay(lease) == B_OK);
	assert(ReleaseDisplay(lease) == B_BAD_VALUE);
	assert(Query(lease, nullptr) == B_BAD_VALUE);
	assert(Allocate(lease, 8192, 4096, 7, &base, &physical) == B_BAD_VALUE);
	assert(Free(lease, 0x10000) == B_BAD_VALUE);
	{
		std::lock_guard<std::mutex> lock(sLock);
		assert(sState.CurrentState() == Owner::RetainedDisplay);
		assert(sState.BeginCleanup(true, true) == Owner::Busy);
		assert(!sState.DiagnosticAllowed());
	}
	{
		std::lock_guard<std::mutex> lock(sBackendLock);
		assert(sCalls == 1);
		sReturn = true;
	}
	sChanged.notify_all();
	worker.join();
	assert(result == sBackendResult);
	if (operation == 1 && !failed)
		assert(base == 0x10000 && physical == 0x20000);
	assert(sCalls == 1);
	std::lock_guard<std::mutex> lock(sLock);
	// This calls only a state-model cleanup probe. No real resource is freed,
	// and true/true is supplied test evidence, not a hardware quiescence claim.
	assert(sState.BeginCleanup(true, true)
		== (failed && operation != 0 ? Owner::Busy : Owner::Success));
}

int
main()
{
	agp_gart_module_info backend = { QueryBackend, AllocateBackend, FreeBackend };
	sGART = &backend;
	for (unsigned operation = 0; operation < 3; operation++) {
		for (unsigned failed = 0; failed < 2; failed++) {
			for (unsigned initializing = 0; initializing < 2; initializing++) {
				RunCase(operation, failed, initializing);
				printf("operation=%u failed=%u initializing=%u passed\n",
					operation, failed, initializing);
			}
		}
	}
	puts("12 actual-adapter cases passed with mocked kernel/GART; no hardware access.");
}
