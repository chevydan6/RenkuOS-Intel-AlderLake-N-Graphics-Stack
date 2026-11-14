// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_CSB_READER_H
#define INTEL_N100_CSB_READER_H
#include "IntelN100ContextStatus.h"
#include <stddef.h>
#include <string.h>

// Gen11/12 engine HWSP, distinct from the context's ppHWSP. Byte offsets.
static constexpr uint32_t kN100CsbEntries = 12;
static constexpr uint32_t kN100CsbBuffer = 0x40;
static constexpr uint32_t kN100CsbWrite = 0xbc;

// CPU-only, unpublished, exclusively owned backing. Caller supplies a writable
// page; this does not establish ownership, coherence or hardware pointer reset.
static inline bool intel_n100_initialize_hwsp(void* page, size_t bytes)
{
	uintptr_t address = reinterpret_cast<uintptr_t>(page);
	if (!page || bytes != 4096 || (address & 7) || address > UINTPTR_MAX - 4095)
		return false;
	memset(page, 0, bytes);
	memset(static_cast<uint8_t*>(page) + kN100CsbBuffer, 0xff, kN100CsbEntries * 8);
	uint32_t tail = kN100CsbEntries - 1;
	memcpy(static_cast<uint8_t*>(page) + kN100CsbWrite, &tail, sizeof(tail));
	return true;
}

// Bounded, one-shot reader for two events from a fresh single-request CSB.
// Backend accesses owned coherent HWSP memory, NOT MMIO. Every callback must
// itself return boundedly. No allocation, publication, cleanup or DMA release.
// External serialization, matching reset/initial cursor, fresh tags, exclusive engine
// ownership and absence of an unseen full FIFO revolution are caller obligations.
class IntelN100CsbReader {
public:
	struct Io {
		void* context;
		uint64_t (*now)(void*); // Monotonic microseconds; UINT64_MAX means failure.
		bool (*safe)(void*); // Live owner/generation, power/mapping and error state.
		bool (*read32)(void*, uint32_t byteOffset, uint32_t&);
		bool (*read64)(void*, uint32_t byteOffset, uint64_t&);
		bool (*acquire)(void*); // Tail->entry read barrier/visibility contract.
		void (*relax)(void*);
	};
	enum Phase { Idle, Polling, EventsObserved, Failed };
	enum Error { None, InvalidInput, Clock, Deadline, Unsafe, TailRead,
		EntryRead, Visibility, UnexpectedTail, TailRegression, EventRejected, PollLimit };
	struct Report {
		Phase phase;
		Error error;
		uint32_t polls, tailReads, entryReads, barriers, safetyChecks;
		uint32_t tail, slot, highestAvailable, initialTail;
		uint64_t entry, start, previous, deadline, elapsed;
		IntelN100ContextStatus::Report events;
	};
	IntelN100CsbReader() : fReport(), fEvents() {}
	const Report& LastReport() const { return fReport; }
	bool Run(const Io& io, uint64_t generation, uint32_t tag,
		uint64_t budgetUs, uint32_t pollLimit, uint32_t initialTail = 11)
	{
		if (fReport.phase != Idle) return false; // Preserve the completed attempt.
		if (!io.now || !io.safe || !io.read32 || !io.read64 || !io.acquire
			|| !io.relax || !budgetUs || budgetUs > 250000 || !pollLimit
			|| pollLimit > 65536 || initialTail >= kN100CsbEntries || !fEvents.Arm(generation, tag))
			return Fail(InvalidInput);
		fReport.events = fEvents.LastReport();
		fReport.initialTail = initialTail;
		fReport.phase = Polling;
		fReport.start = fReport.previous = io.now(io.context);
		if (fReport.start == UINT64_MAX || fReport.start > UINT64_MAX - budgetUs)
			return Fail(Clock);
		fReport.deadline = fReport.start + budgetUs;
		for (uint32_t poll = 0; poll < pollLimit; poll++) {
			fReport.polls++;
			if (!Check(io) || !ReadTail(io)) return false;
			uint32_t available;
			if (!Available(available)) return false;
			if (available > fReport.events.accepted) {
				fReport.barriers++;
				if (!io.acquire(io.context)) return Fail(Visibility);
				if (!Check(io)) return false;
			}
			while (fReport.events.accepted < available) {
				fReport.slot = (fReport.initialTail + 1 + fReport.events.accepted) % kN100CsbEntries;
				fReport.entry = UINT64_MAX;
				if (!Check(io)) return false;
				fReport.entryReads++;
				if (!io.read64(io.context, kN100CsbBuffer + fReport.slot * 8, fReport.entry))
					return Fail(EntryRead);
				if (!Check(io)) return false;
				auto result = fEvents.Observe(generation, fReport.entry);
				fReport.events = fEvents.LastReport();
				if (result == IntelN100ContextStatus::Rejected) return Fail(EventRejected);
				if (result == IntelN100ContextStatus::AwaitingEntryWrite) break;
			}
			if (fReport.events.state == IntelN100ContextStatus::SwitchedAway) {
				// Re-read tail and safety after both entries; an extra event or
				// cursor regression must not be accepted as successful evidence.
				if (!Check(io) || !ReadTail(io) || !Available(available)) return false;
				if (!Check(io)) return false;
				fReport.phase = EventsObserved;
				return true; // CSB evidence only, not permission to free resources.
			}
			if (!Check(io)) return false;
			io.relax(io.context);
		}
		return Fail(PollLimit);
	}
private:
	IntelN100CsbReader(const IntelN100CsbReader&) = delete;
	IntelN100CsbReader& operator=(const IntelN100CsbReader&) = delete;
	bool Fail(Error error)
	{
		fReport.error = error;
		fReport.phase = Failed;
		return false;
	}
	bool ClockValid(const Io& io)
	{
		uint64_t now = io.now(io.context);
		if (now == UINT64_MAX || now < fReport.previous) return Fail(Clock);
		fReport.previous = now;
		fReport.elapsed = now - fReport.start;
		if (now >= fReport.deadline) return Fail(Deadline);
		return true;
	}
	bool Check(const Io& io)
	{
		if (!ClockValid(io)) return false;
		fReport.safetyChecks++;
		if (!io.safe(io.context)) return Fail(Unsafe);
		return ClockValid(io);
	}
	bool ReadTail(const Io& io)
	{
		fReport.tail = UINT32_MAX;
		fReport.tailReads++;
		if (!io.read32(io.context, kN100CsbWrite, fReport.tail)) return Fail(TailRead);
		return Check(io);
	}
	bool Available(uint32_t& available)
	{
		// Exactly two fresh events after the explicitly initialized last-written
		// cursor. Default11 keeps existing callers' slot0/1 behavior; the native
		// N100 owner can explicitly initialize cursor0 and use slots1/2.
		if (fReport.tail >= kN100CsbEntries) return Fail(UnexpectedTail);
		available = (fReport.tail + kN100CsbEntries - fReport.initialTail) % kN100CsbEntries;
		if (available > 2) return Fail(UnexpectedTail);
		if (available < fReport.highestAvailable) return Fail(TailRegression);
		fReport.highestAvailable = available;
		return true;
	}
	Report fReport;
	IntelN100ContextStatus fEvents;
};
#endif
