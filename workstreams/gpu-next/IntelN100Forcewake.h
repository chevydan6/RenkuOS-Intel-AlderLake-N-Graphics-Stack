// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_FORCEWAKE_H
#define INTEL_N100_FORCEWAKE_H
#include <stdint.h>
#include <stddef.h>

// Controlled-boot primary GT handshake only. No DMA, engine access, fallback
// stores, general register input or runtime-PM claim. Owner serializes Run and
// retains this ledger and its operation pin if ownership becomes uncertain.
class IntelN100Forcewake {
public:
	enum Error : uint32_t { Ok, Blocked, InvalidCallbacks, Cancelled,
		PowerChanged, ClockError, ReadError, WriteError, PreTimeout, SetTimeout,
		ClearTimeout, TotalTimeout, FallbackChanged, InvalidMapping, InvariantFailed, InspectFailed, InspectTimeout, AckLost, InspectUncertain };
	enum Stage : uint32_t { Preflight, WaitIdle, Enable, WaitSet, Clear,
		WaitClear, Complete, OwnerValidation, Inspect };
	enum Ownership : uint32_t { NoRequest, EnableAttempted, Issued,
		Acknowledged, ClearAttempted, Released, Unknown };
	struct Report {
		uint32_t error, cleanup, stage, ownership;
		uint32_t initialAck, setAck, finalAck, lastAck, reads, writes;
		uint64_t elapsed;
	};
	struct IO {
		void* context;
		uint64_t (*now)(void*);
		bool (*readAck)(void*, uint32_t&);
		// True means CPU-issued, not device acceptance. Only the exact masked
		// enable/disable words below may reach the fixed request register.
		bool (*writeRequest)(void*, uint32_t);
		bool (*pciStable)(void*);
		bool (*live)(void*);
		void (*pause)(void*);
	};
	IntelN100Forcewake()
		: fBlocked(false), fStart(0), fLastTime(0), fTotalEnd(0), fIO(), fReport() {}
	const Report& LastReport() const { return fReport; }
	bool IsBlocked() const { return fBlocked; }

	Error Run(const IO& io) { return RunWithInspection(io, NULL); }

	// Trusted owner-only fixed sampler. No userspace callback or generic batch.
	enum InspectionResult : uint32_t { InspectionOk, InspectionFailed, InspectionUncertain };
	typedef InspectionResult (*Inspector)(void*, uint64_t deadline);
	Error RunWithInspection(const IO& io, Inspector inspect)
	{
		if (fBlocked)
			return Blocked; // Preserve the original diagnostic ledger.
		fReport = {};
		if (!io.now || !io.readAck || !io.writeRequest || !io.pciStable
			|| !io.live || !io.pause)
			return Fail(InvalidCallbacks);
		fIO = io;
		struct ClearIO {
			IO& io;
			~ClearIO() { io = {}; }
		} clearIO = { fIO }; // No stack context escapes this Run invocation.
		fStart = fLastTime = io.now(io.context);
		if (fStart > UINT64_MAX - 250000)
			return Fail(ClockError);
		fTotalEnd = fStart + 250000;
		if (!io.pciStable(io.context))
			return Fail(PowerChanged);
		Error error = Poll(false, true, WaitIdle, PreTimeout);
		if (error != Ok)
			return Fail(error);
		fReport.initialAck = fReport.lastAck;
		if (!io.live(io.context))
			return Fail(Cancelled);
		if (!io.pciStable(io.context))
			return Fail(PowerChanged);
		if ((error = Time()) != Ok)
			return Fail(error);
		if (fTotalEnd - fLastTime < (inspect != NULL ? 150000u : 100000u))
			return Fail(TotalTimeout); // Reserve set and cleanup wait budgets.

		const uint64_t setDeadline = fLastTime + 50000;
		fReport.stage = Enable;
		fReport.ownership = EnableAttempted; // Persist BEFORE issuing a store.
		fReport.writes++;
		if (!io.writeRequest(io.context, 0x00010001))
			return UnknownFailure(WriteError);
		fReport.ownership = Issued;
		// Mandatory posting observation BEFORE cancellation/deadline polling.
		// Even a close cannot skip this and then guess that clearing is safe.
		error = ReadChecked();
		if (error != Ok)
			return UnknownFailure(error);
		fReport.stage = WaitSet;
		if (fLastTime >= setDeadline)
			error = SetTimeout;
		else if ((fReport.lastAck & 1) == 0)
			error = Poll(true, true, WaitSet, SetTimeout, setDeadline);
		bool acknowledged = error == Ok;
		if (acknowledged) {
			fReport.ownership = Acknowledged;
			fReport.setAck = fReport.lastAck;
		} else if (error != SetTimeout && error != Cancelled) {
			return UnknownFailure(error);
		}
		if (acknowledged && inspect != NULL) {
			if (!io.pciStable(io.context))
				return UnknownFailure(PowerChanged);
			if ((error = Time()) != Ok)
				return UnknownFailure(error);
			if (fTotalEnd - fLastTime < 100000)
				return UnknownFailure(TotalTimeout);
			const uint64_t inspectDeadline = fLastTime + 50000;
			fReport.stage = Inspect;
			if ((error = ReadChecked()) != Ok)
				return UnknownFailure(error);
			if ((fReport.lastAck & 1) == 0)
				return UnknownFailure(AckLost);
			if (fLastTime >= inspectDeadline)
				return UnknownFailure(InspectTimeout);
			if (!io.live(io.context))
				error = Cancelled;
			else {
				InspectionResult inspected = inspect(io.context, inspectDeadline);
				if (!io.pciStable(io.context))
					return UnknownFailure(PowerChanged);
				if ((error = Time()) != Ok)
					return UnknownFailure(error);
				if (fLastTime >= inspectDeadline)
					return UnknownFailure(InspectTimeout);
				if ((error = ReadChecked()) != Ok)
					return UnknownFailure(error);
				if ((fReport.lastAck & 1) == 0)
					return UnknownFailure(AckLost);
				if (fLastTime >= inspectDeadline)
					return UnknownFailure(InspectTimeout);
				if (inspected != InspectionOk && inspected != InspectionFailed)
					return UnknownFailure(InspectUncertain);
				error = inspected == InspectionOk ? Ok : InspectFailed;
				if (!io.live(io.context))
					error = Cancelled;
			}
		}
		// Even an ACK timeout/cancel after a known CPU issue gets a bounded
		// owned-bit clear attempt, but only with a valid clock and PCI state.
		fReport.error = error;
		if (!io.pciStable(io.context))
			return CleanupFailure(PowerChanged);
		Error cleanup = Time();
		if (cleanup != Ok)
			return CleanupFailure(cleanup);
		if (fTotalEnd - fLastTime < 50000)
			return CleanupFailure(TotalTimeout);
		const uint64_t clearDeadline = fLastTime + 50000;
		fReport.stage = Clear;
		fReport.ownership = ClearAttempted;
		fReport.writes++;
		if (!io.writeRequest(io.context, 0x00010000))
			return CleanupFailure(WriteError);
		cleanup = ReadChecked();
		if (cleanup != Ok)
			return CleanupFailure(cleanup);
		fReport.stage = WaitClear;
		if (fLastTime >= clearDeadline)
			cleanup = ClearTimeout;
		else if ((fReport.lastAck & 1) != 0)
			cleanup = Poll(false, false, WaitClear, ClearTimeout, clearDeadline);
		if (cleanup != Ok)
			return CleanupFailure(cleanup);
		fReport.finalAck = fReport.lastAck;
		if (!io.pciStable(io.context))
			return CleanupFailure(PowerChanged);
		if ((cleanup = Time()) != Ok)
			return CleanupFailure(cleanup);
		// ACK-clear without ever seeing ACK-set cannot rule out a late ACK.
		if (!acknowledged)
			return UnknownFailure(error);
		if (!io.live(io.context))
			return UnknownFailure(Cancelled);
		fReport.ownership = Released;
		fReport.stage = Complete;
		return error == Ok ? Ok : Fail(error);
	}

private:
	Error Time()
	{
		uint64_t now = fIO.now(fIO.context);
		if (now == UINT64_MAX || now < fLastTime)
			return ClockError;
		fLastTime = now;
		fReport.elapsed = now - fStart;
		return now >= fTotalEnd ? TotalTimeout : Ok;
	}
	Error Poll(bool set, bool cancellable, Stage stage, Error timeout,
		uint64_t deadline = 0)
	{
		fReport.stage = stage;
		Error error = Time();
		if (error != Ok)
			return error;
		if (deadline == 0) {
			if (fTotalEnd - fLastTime < 50000)
				return TotalTimeout;
			deadline = fLastTime + 50000;
		}
		for (unsigned poll = 0; poll < 512; poll++) {
			if ((error = Time()) != Ok)
				return error;
			if (fLastTime >= deadline)
				return timeout;
			if (cancellable && !fIO.live(fIO.context))
				return Cancelled;
			error = ReadChecked();
			if (error != Ok)
				return error;
			if (fLastTime >= deadline)
				return timeout;
			if (((fReport.lastAck & 1) != 0) == set)
				return Ok;
			fIO.pause(fIO.context);
		}
		return timeout; // Also bounds a broken/frozen clock or pause callback.
	}
	Error ReadChecked()
	{
		uint32_t ack = 0;
		fReport.reads++;
		if (!fIO.readAck(fIO.context, ack))
			return ReadError;
		fReport.lastAck = ack;
		if (ack == UINT32_MAX)
			return ReadError;
		if ((ack & 0x8000) != 0)
			return FallbackChanged;
		return Time();
	}
	Error Fail(Error error)
	{
		fBlocked = true;
		fReport.error = error;
		return error;
	}
	Error UnknownFailure(Error error)
	{
		fReport.ownership = Unknown;
		return Fail(error);
	}
	Error CleanupFailure(Error error)
	{
		fReport.cleanup = error;
		Error primary = Error(fReport.error);
		return UnknownFailure(primary != Ok ? primary : error);
	}
	IntelN100Forcewake(const IntelN100Forcewake&) = delete;
	IntelN100Forcewake& operator=(const IntelN100Forcewake&) = delete;
	bool fBlocked;
	uint64_t fStart, fLastTime, fTotalEnd;
	IO fIO;
	Report fReport;
};
#endif
