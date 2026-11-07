// SPDX-License-Identifier: MIT
#ifndef INTEL_APERTURE_LEASE_STATE_H
#define INTEL_APERTURE_LEASE_STATE_H

#include <stddef.h>
#include <stdint.h>

// Memory-only lifetime bookkeeping for the retained N100 owner. All methods,
// including queries, require the owner's mutex. This class performs no I/O and
// supplies no GPU publication or quiescence proof. The containing module must
// retain its record and resources until FinishCleanup succeeds.
class IntelApertureLeaseState {
public:
	enum State { Fresh, Initializing, Ready, RetainedDisplay, Closing,
		Quarantined, Closed };
	enum Result { Success, Invalid, Busy, Exhausted };
	enum Kind { Display, Request };
	enum Access { Query, Allocate, Free };

	class Operation {
	public:
		// The containing adapter must end each begun ticket before destroying
		// it, including every error path. Abandoning a ticket retains its pin
		// indefinitely; the destructor deliberately cannot perform unlocked I/O.
		Operation() : fOwner(NULL), fLease(0) {}
	private:
		friend class IntelApertureLeaseState;
		Operation(const Operation&) = delete;
		Operation& operator=(const Operation&) = delete;
		IntelApertureLeaseState* fOwner;
		uint64_t fLease;
	};

	IntelApertureLeaseState()
		: fState(Fresh), fNextID(0), fDiagnosticBlocked(false),
		  fUnresolvedBacking(false)
	{
		for (unsigned i = 0; i < 2; i++) {
			fLeases[i].id = 0;
			fLeases[i].operations = 0;
			fLeases[i].live = false;
		}
	}

	Result BeginInitialization(uint64_t& displayLease)
	{
		if (fState != Fresh && fState != Closed)
			return Busy;
		Result result = Acquire(Display, displayLease);
		if (result != Success)
			return result;
		fDiagnosticBlocked = true;
		fState = Initializing;
		return Success;
	}

	Result FinishInitialization(uint64_t displayLease, bool success)
	{
		if (fState != Initializing || !IsLive(Display, displayLease))
			return Invalid;
		if (fLeases[Display].operations != 0)
			return Busy;
		// Failure can leave firmware scanout or partially acquired backing.
		// Retain ownership until the containing module proves cleanup safe.
		fState = success ? Ready : RetainedDisplay;
		fDiagnosticBlocked = !success || fUnresolvedBacking;
		return Success;
	}

	// Revoke the caller's initializing lease without discarding in-flight pins.
	// This is not successful hardware shutdown or resource reclamation.
	Result AbortInitialization(uint64_t displayLease)
	{
		if (fState != Initializing || !IsLive(Display, displayLease))
			return Invalid;
		fLeases[Display].live = false;
		fState = RetainedDisplay;
		fDiagnosticBlocked = true;
		return Success;
	}

	Result AcquireRequest(uint64_t& requestLease)
	{
		if (!DiagnosticAllowed())
			return Busy;
		return Acquire(Request, requestLease);
	}

	Result Release(uint64_t lease)
	{
		if (fState == Initializing)
			return Busy;
		Lease* record = Find(lease);
		if (record == NULL || !record->live)
			return Invalid;
		record->live = false;
		if (record == &fLeases[Display]) {
			fState = RetainedDisplay;
			fDiagnosticBlocked = true;
		}
		return Success;
	}

	Result BeginOperation(uint64_t lease, Access access, Operation& operation)
	{
		if (access != Query && access != Allocate && access != Free)
			return Invalid;
		Lease* record = Find(lease);
		if (record == NULL || !record->live || operation.fOwner != NULL)
			return Invalid;
		if (fState == Closing || fState == Closed)
			return Busy;
		// Only the initializing display lease can forward GART calls here.
		// FinishInitialization and Release cannot race past active tickets.
		if (fState == Initializing && record != &fLeases[Display])
			return Busy;
		if (access == Allocate && ((fState != Ready && fState != Initializing)
			|| (record == &fLeases[Request] && fDiagnosticBlocked)))
			return Busy;
		if (record->operations == UINT64_MAX)
			return Exhausted;
		record->operations++;
		operation.fOwner = this;
		operation.fLease = lease;
		return Success;
	}

	Result EndOperation(Operation& operation, bool backingUnresolved)
	{
		if (operation.fOwner != this)
			return Invalid;
		Lease* record = Find(operation.fLease);
		if (record == NULL || record->operations == 0)
			return Invalid;
		// Must occur under the same mutex, BEFORE dropping the operation pin.
		// Covers hidden allocation rollback and failed-free/unbind retention.
		if (backingUnresolved) {
			fDiagnosticBlocked = true;
			fUnresolvedBacking = true;
		}
		record->operations--;
		operation.fOwner = NULL;
		operation.fLease = 0;
		return Success;
	}

	Result BeginCleanup(bool displayQuiescent, bool gpuQuiescent)
	{
		if (fState == Fresh || fState == Initializing || fState == Closing
			|| fState == Closed || fState == Quarantined)
			return Busy;
		// Quiescence alone cannot resolve hidden partial-bind ownership.
		// No in-place reclamation protocol exists for this condition yet.
		if (fUnresolvedBacking || !displayQuiescent || !gpuQuiescent)
			return Busy;
		for (unsigned i = 0; i < 2; i++) {
			if (fLeases[i].live || fLeases[i].operations != 0)
				return Busy;
		}
		fState = Closing;
		fDiagnosticBlocked = true;
		return Success;
	}

	Result FinishCleanup(bool success)
	{
		if (fState != Closing)
			return Invalid;
		fState = success ? Closed : Quarantined;
		return Success;
	}

	bool DiagnosticAllowed() const
	{
		return fState == Ready && !fDiagnosticBlocked && fLeases[Display].live;
	}

	State CurrentState() const { return fState; }

private:
	struct Lease {
		uint64_t id;
		uint64_t operations;
		bool live;
	};

	Result Acquire(Kind kind, uint64_t& output)
	{
		Lease& lease = fLeases[kind];
		if (lease.live || lease.operations != 0)
			return Busy;
		if (fNextID == UINT64_MAX)
			return Exhausted;
		lease.id = ++fNextID;
		lease.live = true;
		output = lease.id;
		return Success;
	}

	bool IsLive(Kind kind, uint64_t id) const
	{
		return id != 0 && fLeases[kind].id == id && fLeases[kind].live;
	}

	Lease* Find(uint64_t id)
	{
		if (id == 0)
			return NULL;
		for (unsigned i = 0; i < 2; i++) {
			if (fLeases[i].id == id)
				return &fLeases[i];
		}
		return NULL;
	}

	IntelApertureLeaseState(const IntelApertureLeaseState&) = delete;
	IntelApertureLeaseState& operator=(const IntelApertureLeaseState&) = delete;
	State fState;
	uint64_t fNextID;
	bool fDiagnosticBlocked;
	bool fUnresolvedBacking;
	Lease fLeases[2];
};

#endif
