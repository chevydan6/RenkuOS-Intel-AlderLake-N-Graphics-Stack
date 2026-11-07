// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_BCS_RESET_H
#define INTEL_N100_BCS_RESET_H
#include <stdint.h>

// Fixed reset-only algorithm; not connected to hardware. See BCS-RESET-DESIGN.md.
// Caller retains this single-use ledger and serializes all access. A successful
// reset observation is NOT engine initialization or permission to release DMA.
class IntelN100BcsReset {
public:
	static constexpr uint32_t kControl = 0x2203c, kMode = 0x2209c;
	static constexpr uint32_t kPrefetch = 0x2229c, kPending = 0x800c;
	static constexpr uint32_t kResetControl = 0x220d0, kDomain = 0x941c;
	enum Error : uint32_t { Ok, AlreadyUsed, InvalidIO, Unsafe, ClockError,
		Deadline, ReadError, WriteError, PreexistingState, PendingWake, Timeout };
	enum Stage : uint32_t { Preflight, Stop, DisablePrefetch, WaitIdle,
		CheckPending, RequestReset, WaitReady, ResetFirst, ResetSecond,
		Settle, CancelRequest, ResetObserved };
	struct Report {
		uint32_t error, stage, reads, writes, resetRounds;
		uint32_t lastOffset, lastValue, attemptedOffset, attemptedValue;
		uint64_t elapsed;
	};
	struct IO {
		void* context;
		uint64_t (*now)(void*);
		bool (*safe)(void*); // Identity, power/forcewake, mapping and owner valid.
		bool (*read)(void*, uint32_t, uint32_t&);
		bool (*write)(void*, uint32_t, uint32_t); // False may still have issued.
		void (*pause)(void*); // Bounded; clock and iteration caps are independent.
	};
	IntelN100BcsReset() : fUsed(false), fIO(), fReport(), fStart(0), fLast(0) {}
	const Report& LastReport() const { return fReport; }
	bool WasUsed() const { return fUsed; }

	Error Run(const IO& io)
	{
		if (fUsed)
			return AlreadyUsed;
		fUsed = true;
		if (!io.now || !io.safe || !io.read || !io.write || !io.pause)
			return Fail(InvalidIO);
		fIO = io;
		struct ClearIO { IO& io; ~ClearIO() { io = {}; } } clear = { fIO };
		fStart = fLast = io.now(io.context);
		if (fStart > UINT64_MAX - 10000)
			return Fail(ClockError);
		if (!Read(kControl) || !Require((fReport.lastValue & 1) == 0)
			|| !Read(kMode) || !Require((fReport.lastValue & 0x300) == 0x200)
			|| !Read(kPrefetch) || !Require((fReport.lastValue & 0x400) == 0)
			|| !Read(kResetControl) || !Require((fReport.lastValue & 7) == 0)
			|| !Read(kDomain) || !Require(fReport.lastValue == 0))
			return Error(fReport.error);
		if (!Store(Stop, kMode, 0x01000100)
			|| !Store(DisablePrefetch, kPrefetch, 0x04000400)
			|| !Poll(WaitIdle, kMode, 0x300, 0x300, 1000))
			return Error(fReport.error);
		// Posting observation, independently checked even after MODE_IDLE.
		if (!Read(kMode) || !Require((fReport.lastValue & 0x300) == 0x300))
			return Error(fReport.error);
		fReport.stage = CheckPending;
		if (!Read(kPending))
			return Error(fReport.error);
		uint32_t pending = fReport.lastValue;
		if ((pending & (pending >> 16) & 0x3e00) != 0)
			return Fail(PendingWake);
		// Do not overwrite state that changed during stop/prefetch preparation.
		if (!Read(kResetControl) || !Require((fReport.lastValue & 7) == 0)
			|| !Store(RequestReset, kResetControl, 0x00010001)
			|| !Poll(WaitReady, kResetControl, 3, 3, 700, 4))
			return Error(fReport.error);
		for (unsigned round = 0; round < 2; round++) {
			if (!Read(kDomain) || !Require(fReport.lastValue == 0)
				|| !Store(round == 0 ? ResetFirst : ResetSecond, kDomain, 4)
				|| !Poll(round == 0 ? ResetFirst : ResetSecond, kDomain, 4, 0, 2000, ~4u))
				return Error(fReport.error);
			fReport.resetRounds++;
		}
		fReport.stage = Settle;
		uint64_t until = fLast + 50;
		bool settled = false;
		for (unsigned count = 0; count < 4096; count++) {
			if (!Check())
				return Error(fReport.error);
			if (fLast >= until) { settled = true; break; }
			io.pause(io.context);
		}
		if (!settled)
			return Fail(Timeout);
		if (!Store(CancelRequest, kResetControl, 0x00010000)
			|| !Poll(CancelRequest, kResetControl, 5, 0, 700))
			return Error(fReport.error);
		fReport.stage = ResetObserved;
		return Ok;
	}

private:
	Error Fail(Error error) { fReport.error = error; return error; }
	bool Require(bool condition)
	{
		if (condition) return true;
		Fail(PreexistingState); return false;
	}
	bool Time()
	{
		uint64_t now = fIO.now(fIO.context);
		if (now == UINT64_MAX || now < fLast) { Fail(ClockError); return false; }
		fLast = now;
		fReport.elapsed = now - fStart;
		if (fReport.elapsed >= 10000) { Fail(Deadline); return false; }
		return true;
	}
	bool Check()
	{
		if (!Time()) return false;
		if (!fIO.safe(fIO.context)) { Fail(Unsafe); return false; }
		return Time(); // Include time spent validating ownership/power.
	}
	bool Read(uint32_t offset)
	{
		if (!Check()) return false;
		fReport.reads++;
		fReport.lastOffset = offset;
		uint32_t value = 0;
		if (!fIO.read(fIO.context, offset, value)) { Fail(ReadError); return false; }
		fReport.lastValue = value;
		if (value == UINT32_MAX) { Fail(ReadError); return false; }
		return Check();
	}
	bool Store(Stage stage, uint32_t offset, uint32_t value)
	{
		fReport.stage = stage;
		if (!Check()) return false;
		// Attempt ledger precedes the callback, including ambiguous failures.
		fReport.attemptedOffset = offset;
		fReport.attemptedValue = value;
		fReport.writes++;
		if (!fIO.write(fIO.context, offset, value)) { Fail(WriteError); return false; }
		return Check();
	}
	bool Poll(Stage stage, uint32_t offset, uint32_t mask, uint32_t value,
		uint64_t budget, uint32_t forbidden = 0)
	{
		fReport.stage = stage;
		const uint64_t end = fLast + budget;
		for (unsigned count = 0; count < 4096; count++) {
			if (!Check()) return false;
			if (fLast >= end) { Fail(Timeout); return false; }
			if (!Read(offset)) return false;
			if (fLast >= end) { Fail(Timeout); return false; }
			if (!Require((fReport.lastValue & forbidden) == 0)) return false;
			if ((fReport.lastValue & mask) == value) return true;
			fIO.pause(fIO.context);
		}
		Fail(Timeout); return false;
	}
	IntelN100BcsReset(const IntelN100BcsReset&) = delete;
	IntelN100BcsReset& operator=(const IntelN100BcsReset&) = delete;
	bool fUsed;
	IO fIO;
	Report fReport;
	uint64_t fStart, fLast;
};
#endif
