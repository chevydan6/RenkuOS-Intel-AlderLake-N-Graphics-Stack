// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_FORCEWAKE_GROUP_H
#define INTEL_N100_FORCEWAKE_GROUP_H
#include "IntelN100ResetDomains.h"
#include <stddef.h>

// Trusted retained-owner helper; not yet connected to MMIO. Single use, no
// fallback-bit stores and no speculative cleanup on any failure. The owner must
// retain this ledger, its code/mapping and operation pin whenever Run fails.
class IntelN100ForcewakeGroup {
public:
	enum Error : uint32_t { Ok, AlreadyUsed, InvalidIO, InvalidPlan, Unsafe,
		ClockError, TotalTimeout, ReadError, WriteError, FallbackBusy,
		PreexistingRequest, AckTimeout, AckLost, WorkFailed, WorkTimeout };
	enum Stage : uint32_t { Preflight, Acquire, VerifyHeld, Work, Release, VerifyReleased, Complete };
	enum Ownership : uint32_t { NoRequest, EnableAttempted, Issued, Held, ClearAttempted, Released };
	struct DomainReport {
		uint32_t kind, request, ack, ownership, reads, writes;
		uint32_t initialAck, setAck, finalAck, lastAck;
	};
	struct Report {
		uint32_t error, stage, domain, count, workCalled;
		uint64_t elapsed;
		DomainReport domains[5];
	};
	struct IO {
		void* context;
		uint64_t (*now)(void*);
		bool (*safe)(void*); // Owner, exact PCI/D0 identity and retained UC map.
		bool (*readAck)(void*, uint32_t, uint32_t&);
		bool (*writeRequest)(void*, uint32_t, uint32_t);
		void (*pause)(void*);
	};
	// Future trusted reset callback only. False retains every wake request;
	// failure is NOT permission to release power or DMA. No userspace callbacks.
	typedef bool (*Operation)(void*, uint64_t deadline);
	IntelN100ForcewakeGroup() : fUsed(false), fIO(), fReport(), fStart(0), fLast(0) {}
	const Report& LastReport() const { return fReport; }
	Error Run(const IO& io, uint32_t identity, uint8_t revision, uint32_t rawFuse,
		Operation operation = NULL)
	{
		if (fUsed) return AlreadyUsed;
		fUsed = true;
		if (!io.now || !io.safe || !io.readAck || !io.writeRequest || !io.pause)
			return Fail(InvalidIO);
		IntelN100ResetDomains plan = {};
		if (!intel_n100_reset_domains(identity, revision, rawFuse, plan))
			return Fail(InvalidPlan);
		fReport.count = plan.count;
		for (unsigned i = 0; i < plan.count; i++) {
			fReport.domains[i].kind = plan.domains[i].kind;
			fReport.domains[i].request = plan.domains[i].request;
			fReport.domains[i].ack = plan.domains[i].ack;
		}
		fIO = io;
		struct ClearIO { IO& io; ~ClearIO() { io = {}; } } clear = { fIO };
		fStart = fLast = io.now(io.context);
		if (fStart > UINT64_MAX - 1000000) return Fail(ClockError);
		// Read every selected ACK before issuing anything. Never clear a stale
		// kernel/fallback request on the assumption that it belongs to us.
		for (unsigned i = 0; i < plan.count; i++) {
			if (!Read(i)) return Error(fReport.error);
			fReport.domains[i].initialAck = fReport.domains[i].lastAck;
			if (fReport.domains[i].lastAck & 1) return Fail(PreexistingRequest);
		}
		for (unsigned i = 0; i < plan.count; i++) {
			fReport.stage = Acquire;
			if (!Read(i)) return Error(fReport.error);
			if (fReport.domains[i].lastAck & 1) return Fail(PreexistingRequest);
			if (!Store(i, true) || !Poll(i, true)) return Error(fReport.error);
			fReport.domains[i].ownership = Held;
			fReport.domains[i].setAck = fReport.domains[i].lastAck;
			if (!Verify(i + 1, true)) return Error(fReport.error);
		}
		fReport.stage = VerifyHeld;
		if (!Verify(plan.count, true)) return Error(fReport.error);
		if (operation != NULL) {
			fReport.stage = Work;
			if (!Check()) return Error(fReport.error);
			if (1000000 - (fLast - fStart) < 400000) return Fail(TotalTimeout);
			const uint64_t deadline = fLast + 100000;
			fReport.workCalled = 1;
			bool success = operation(io.context, deadline);
			if (!Check()) return Error(fReport.error);
			if (fLast >= deadline) return Fail(WorkTimeout);
			if (!success) return Fail(WorkFailed);
			if (!Verify(plan.count, true)) return Error(fReport.error);
		}
		// Release the exact acquired set in reverse. One failed clear stops
		// immediately, retaining the remaining known/attempted ownership.
		for (unsigned n = plan.count; n > 0; n--) {
			fReport.stage = Release;
			if (!Verify(n, true)) return Error(fReport.error);
			if (!Store(n - 1, false) || !Poll(n - 1, false)) return Error(fReport.error);
			fReport.domains[n - 1].ownership = Released;
			fReport.domains[n - 1].finalAck = fReport.domains[n - 1].lastAck;
		}
		fReport.stage = VerifyReleased;
		if (!Verify(plan.count, false)) return Error(fReport.error);
		fReport.stage = Complete;
		return Ok;
	}

private:
	Error Fail(Error error) { fReport.error = error; return error; }
	bool Time()
	{
		uint64_t now = fIO.now(fIO.context);
		if (now == UINT64_MAX || now < fLast) { Fail(ClockError); return false; }
		fLast = now; fReport.elapsed = now - fStart;
		if (fReport.elapsed >= 1000000) { Fail(TotalTimeout); return false; }
		return true;
	}
	bool Check()
	{
		if (!Time()) return false;
		if (!fIO.safe(fIO.context)) { Fail(Unsafe); return false; }
		return Time();
	}
	bool Read(unsigned index)
	{
		fReport.domain = index;
		if (!Check()) return false;
		auto& d = fReport.domains[index];
		d.reads++;
		uint32_t value = 0;
		if (!fIO.readAck(fIO.context, d.ack, value)) { Fail(ReadError); return false; }
		d.lastAck = value;
		if (value == UINT32_MAX) { Fail(ReadError); return false; }
		if (value & 0x8000) { Fail(FallbackBusy); return false; }
		return Check();
	}
	bool Store(unsigned index, bool enable)
	{
		fReport.domain = index;
		if (!Check()) return false;
		auto& d = fReport.domains[index];
		d.ownership = enable ? EnableAttempted : ClearAttempted;
		d.writes++;
		if (!fIO.writeRequest(fIO.context, d.request, enable ? 0x10001 : 0x10000)) {
			Fail(WriteError); return false;
		}
		if (enable) d.ownership = Issued;
		return true; // Next poll validates safety and observes ACK before release.
	}
	bool Poll(unsigned index, bool set)
	{
		const uint64_t deadline = fLast + 50000;
		for (unsigned tries = 0; tries < 512; tries++) {
			if (!Read(index)) return false;
			if (fLast >= deadline) { Fail(AckTimeout); return false; }
			if (bool(fReport.domains[index].lastAck & 1) == set) return true;
			fIO.pause(fIO.context);
		}
		Fail(AckTimeout); return false;
	}
	bool Verify(unsigned count, bool held)
	{
		for (unsigned i = 0; i < count; i++) {
			if (!Read(i)) return false;
			if (bool(fReport.domains[i].lastAck & 1) != held) { Fail(AckLost); return false; }
		}
		return true;
	}
	IntelN100ForcewakeGroup(const IntelN100ForcewakeGroup&) = delete;
	IntelN100ForcewakeGroup& operator=(const IntelN100ForcewakeGroup&) = delete;
	bool fUsed;
	IO fIO;
	Report fReport;
	uint64_t fStart, fLast;
};
#endif
