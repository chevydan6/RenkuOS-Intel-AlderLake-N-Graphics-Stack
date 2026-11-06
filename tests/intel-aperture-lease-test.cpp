// SPDX-License-Identifier: MIT
#include "IntelApertureLeaseState.h"
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <initializer_list>
#include <mutex>
#include <thread>

using Owner = IntelApertureLeaseState;

static uint64_t
Initialize(Owner& owner)
{
	uint64_t lease = 0;
	assert(owner.BeginInitialization(lease) == Owner::Success && lease != 0);
	assert(owner.Release(lease) == Owner::Busy);
	assert(owner.FinishInitialization(lease, true) == Owner::Success);
	return lease;
}

int
main()
{
	// Initialization failure, last-close retention and explicit idle evidence.
	{
		Owner owner;
		uint64_t display = 0, request = 91;
		assert(owner.BeginInitialization(display) == Owner::Success);
		assert(owner.AcquireRequest(request) == Owner::Busy && request == 91);
		assert(owner.FinishInitialization(display, false) == Owner::Success);
		assert(owner.Release(display) == Owner::Success);
		assert(owner.BeginInitialization(request) == Owner::Busy);
		assert(owner.BeginCleanup(false, true) == Owner::Busy);
		assert(owner.BeginCleanup(true, false) == Owner::Busy);
		assert(owner.BeginCleanup(true, true) == Owner::Success);
		assert(owner.FinishCleanup(false) == Owner::Success);
		assert(owner.CurrentState() == Owner::Quarantined);
		assert(owner.BeginCleanup(true, true) == Owner::Busy);
		assert(owner.BeginInitialization(request) == Owner::Busy);
	}
	// Failed owned allocation blocks diagnostic admission, preserving display
	// cleanup; released cookies and operation tickets cannot be replayed.
	{
		Owner owner;
		uint64_t display = Initialize(owner), request = 0, next = 91;
		assert(owner.AcquireRequest(request) == Owner::Success);
		assert(owner.AcquireRequest(next) == Owner::Busy && next == 91);
		Owner::Operation failed, cleanup;
		assert(owner.BeginOperation(display, Owner::Allocate, failed) == Owner::Success);
		assert(owner.EndOperation(failed, true) == Owner::Success);
		assert(!owner.DiagnosticAllowed());
		assert(owner.BeginOperation(request, Owner::Allocate, failed) == Owner::Busy);
		assert(owner.BeginOperation(display, Owner::Free, cleanup) == Owner::Success);
		assert(owner.Release(display) == Owner::Success);
		assert(owner.Release(display) == Owner::Invalid);
		assert(owner.Release(request) == Owner::Success);
		assert(owner.BeginCleanup(true, true) == Owner::Busy);
		assert(owner.EndOperation(cleanup, false) == Owner::Success);
		assert(owner.EndOperation(cleanup, false) == Owner::Invalid);
		assert(owner.BeginOperation(display, Owner::Free, cleanup) == Owner::Invalid);
		assert(owner.BeginCleanup(true, true) == Owner::Busy);
		assert(owner.FinishCleanup(true) == Owner::Invalid);
	}
	// Admission is externally mutex protected. Release between operation pin
	// and backend return must neither allow cleanup nor permit a same-boot reopen.
	for (unsigned round = 0; round < 100; round++) {
		Owner owner;
		std::mutex mutex;
		std::condition_variable changed;
		bool entered = false, released = false;
		uint64_t display = Initialize(owner);
		std::thread operation([&] {
			Owner::Operation ticket;
			{
				std::unique_lock<std::mutex> lock(mutex);
				assert(owner.BeginOperation(display, Owner::Allocate, ticket) == Owner::Success);
				entered = true;
				changed.notify_all();
				assert(changed.wait_for(lock, std::chrono::seconds(5), [&] { return released; }));
			}
			// A real backend call returns here, outside the owner mutex.
			std::lock_guard<std::mutex> lock(mutex);
			assert(owner.EndOperation(ticket, true) == Owner::Success);
			assert(!owner.DiagnosticAllowed());
		});
		{
			std::unique_lock<std::mutex> lock(mutex);
			assert(changed.wait_for(lock, std::chrono::seconds(5), [&] { return entered; }));
			assert(owner.Release(display) == Owner::Success);
			uint64_t unused = 73;
			assert(owner.BeginInitialization(unused) == Owner::Busy && unused == 73);
			assert(owner.BeginCleanup(true, true) == Owner::Busy);
			released = true;
		}
		changed.notify_all();
		operation.join();
		assert(owner.BeginCleanup(true, true) == Owner::Busy);
	}
	// Clean teardown/recreation rejects old numeric leases.
	{
		Owner owner;
		uint64_t display = Initialize(owner), request = 0;
		assert(owner.AcquireRequest(request) == Owner::Success);
		assert(owner.Release(request) == Owner::Success);
		assert(owner.Release(display) == Owner::Success);
		assert(owner.BeginCleanup(true, true) == Owner::Success);
		assert(owner.FinishCleanup(true) == Owner::Success);
		uint64_t recreated = Initialize(owner);
		assert(recreated != display && recreated != request);
		Owner::Operation ticket;
		assert(owner.Release(display) == Owner::Invalid);
		assert(owner.BeginOperation(request, Owner::Query, ticket) == Owner::Invalid);
		assert(owner.BeginOperation(recreated, static_cast<Owner::Access>(99), ticket)
			== Owner::Invalid);
	}
	// Initialization may forward all display operations, but neither finish
	// nor release may escape an outstanding operation. Failure stays sticky.
	for (unsigned outcome = 0; outcome < 4; outcome++) {
		Owner owner;
		uint64_t display = 0, request = 0;
		assert(owner.BeginInitialization(display) == Owner::Success);
		for (Owner::Access access : {Owner::Query, Owner::Allocate, Owner::Free}) {
			Owner::Operation ticket;
			assert(owner.BeginOperation(display, access, ticket) == Owner::Success);
			assert(owner.FinishInitialization(display, true) == Owner::Busy);
			assert(owner.FinishInitialization(display, false) == Owner::Busy);
			assert(owner.Release(display) == Owner::Busy);
			assert(owner.AcquireRequest(request) == Owner::Busy);
			assert(owner.EndOperation(ticket, access == Owner::Allocate && (outcome & 1))
				== Owner::Success);
		}
		bool initialized = (outcome & 2) != 0;
		assert(owner.FinishInitialization(display, initialized) == Owner::Success);
		assert(owner.DiagnosticAllowed() == (initialized && !(outcome & 1)));
		assert(owner.Release(display) == Owner::Success);
		assert(owner.BeginCleanup(true, true)
			== ((outcome & 1) ? Owner::Busy : Owner::Success));
	}
	// A request allocation failure also retains potentially hidden backing.
	{
		Owner owner;
		uint64_t display = Initialize(owner), request = 0;
		assert(owner.AcquireRequest(request) == Owner::Success);
		Owner::Operation ticket;
		assert(owner.BeginOperation(request, Owner::Allocate, ticket) == Owner::Success);
		assert(owner.EndOperation(ticket, true) == Owner::Success);
		assert(owner.Release(request) == Owner::Success);
		assert(owner.Release(display) == Owner::Success);
		assert(owner.BeginCleanup(true, true) == Owner::Busy);
	}
	// Abort revokes only an initializing display lease, retaining every ticket.
	for (bool failure : {false, true}) {
		Owner owner;
		uint64_t display = 0;
		assert(owner.BeginInitialization(display) == Owner::Success);
		Owner::Operation ticket;
		assert(owner.BeginOperation(display, Owner::Allocate, ticket) == Owner::Success);
		assert(owner.AbortInitialization(display + 1) == Owner::Invalid);
		assert(owner.AbortInitialization(display) == Owner::Success);
		assert(owner.AbortInitialization(display) == Owner::Invalid);
		assert(owner.Release(display) == Owner::Invalid);
		assert(owner.FinishInitialization(display, true) == Owner::Invalid);
		assert(owner.BeginCleanup(true, true) == Owner::Busy);
		Owner::Operation rejected;
		assert(owner.BeginOperation(display, Owner::Free, rejected) == Owner::Invalid);
		assert(owner.EndOperation(ticket, failure) == Owner::Success);
		assert(owner.EndOperation(ticket, failure) == Owner::Invalid);
		assert(!owner.DiagnosticAllowed());
		assert(owner.BeginCleanup(false, true) == Owner::Busy);
		assert(owner.BeginCleanup(true, true) == (failure ? Owner::Busy : Owner::Success));
	}
	{
		Owner owner;
		uint64_t display = Initialize(owner), request = 0;
		assert(owner.AcquireRequest(request) == Owner::Success);
		assert(owner.AbortInitialization(request) == Owner::Invalid);
		assert(owner.AbortInitialization(display) == Owner::Invalid);
	}
	// A single cleanup winner even when two final releases contend.
	{
		Owner owner;
		uint64_t display = Initialize(owner);
		std::mutex mutex;
		unsigned winners = 0;
		auto close = [&] {
			std::lock_guard<std::mutex> lock(mutex);
			if (owner.Release(display) == Owner::Success) {
				assert(owner.BeginCleanup(true, true) == Owner::Success);
				winners++;
			}
		};
		std::thread first(close), second(close);
		first.join(); second.join();
		assert(winners == 1 && owner.CurrentState() == Owner::Closing);
	}
	puts("Lease transitions and 100 close/operation races passed (state-only harness)");
}
