// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_CONTEXT_STATUS_H
#define INTEL_N100_CONTEXT_STATUS_H
#include <stdint.h>

// Offline Gen12 (pre-XeHP) BCS0/RCS0 helpers. No allocation, MMIO or publication.
// Source/field review: research/n100-submission/CONTEXT-STATUS-DESIGN.md.
static inline bool
intel_n100_bcs_descriptor(uint32_t identity, uint8_t revision,
	uint64_t contextGgtt, uint32_t tag, uint64_t* output)
{
	// Require room for ppHWSP + register image + two Gen12 workaround pages.
	// This only validates arithmetic, not ownership, contents or DMA readiness.
	if (!output || identity != 0x46d18086 || revision != 0
		|| (contextGgtt & 4095) || contextGgtt > (1ULL << 32) - 16384
		|| tag == 0 || tag >= 0x7ff)
		return false;
	// VALID | FORCE_RESTORE | legacy64 addressing(3 << 3) | PRIVILEGE.
	// HW COPY_ENGINE_CLASS=3, instance=0, counter=0. No EU priority flags.
	*output = contextGgtt | 0x11dULL | (uint64_t(tag) << 37) | (3ULL << 61);
	return true;
}

// RCS0 fresh backing: ppHWSP + 13 register-image pages + two WA pages.
// Arithmetic only; neither this descriptor nor its construction grants ownership.
static inline bool
intel_n100_rcs_descriptor(uint32_t identity, uint8_t revision,
	uint64_t contextGgtt, uint32_t tag, uint64_t* output)
{
	if (!output || identity != 0x46d18086 || revision != 0
		|| (contextGgtt & 4095) || contextGgtt > (1ULL << 32) - 65536
		|| tag == 0 || tag >= 0x7ff)
		return false;
	// Same addressing/restore flags as BCS, HW RENDER_ENGINE_CLASS=0,
	// instance=0, counter=0. No EU priority flags.
	*output = contextGgtt | 0x11dULL | (uint64_t(tag) << 37);
	return true;
}

struct IntelN100CsbEvent {
	uint64_t raw;
	uint32_t toTag, awayTag, toCounter, awayCounter;
	uint32_t engineClass, instance, detail, waitDetail;
	bool newQueue, semaphoreMode;
};

static inline IntelN100CsbEvent
intel_n100_csb_decode(uint64_t raw)
{
	return {raw, uint32_t((raw >> 15) & 0x7ff), uint32_t((raw >> 47) & 0x7ff),
		uint32_t((raw >> 26) & 0x3f), uint32_t((raw >> 58) & 0x3f),
		uint32_t((raw >> 3) & 7), uint32_t((raw >> 6) & 0x3f),
		uint32_t((raw >> 32) & 15), uint32_t((raw >> 37) & 0x7f),
		(raw & 1) != 0, (raw & 4) != 0};
}

// One-shot software ledger for an isolated, initially idle BCS0 or RCS0, one descriptor,
// zero counter, no preemption or resubmission. Serialize every call. The caller
// must establish visibility, CSB order/no overrun, deadline and error handling.
// Generation is a software envelope, not a generation encoded by the hardware.
// Never reuse tags/backing until an external retirement/reset contract permits.
// SwitchedAway is CSB evidence ONLY: it cannot authorize freeing GPU resources.
class IntelN100ContextStatus {
public:
	enum State { Empty, AwaitingActivation, Active, SwitchedAway, Uncertain };
	enum Error { None, NotArmed, WrongGeneration, WrongEngine, WrongTag,
		WrongCounter, UnexpectedTransition };
	enum Result { Accepted, AwaitingEntryWrite, Rejected };
	struct Report {
		State state;
		Error error;
		uint64_t generation, raw[2], attemptedRaw;
		uint32_t tag, accepted;
	};
	IntelN100ContextStatus() : fReport(), fEngineClass(3) {}
	const Report& LastReport() const { return fReport; }
	bool Arm(uint64_t generation, uint32_t tag, uint32_t engineClass = 3)
	{
		if (fReport.state != Empty || !generation || !tag || tag >= 0x7ff
			|| (engineClass != 0 && engineClass != 3))
			return false;
		fEngineClass = engineClass; // Explicit software expectation, never MMIO authority.
		fReport.generation = generation;
		fReport.tag = tag;
		fReport.state = AwaitingActivation;
		return true;
	}
	Result Observe(uint64_t generation, uint64_t raw)
	{
		if (fReport.state == Uncertain) return Rejected; // Preserve first failure.
		fReport.attemptedRaw = raw;
		if (fReport.state == Empty) return Fail(NotArmed);
		if (generation != fReport.generation) return Fail(WrongGeneration);
		if (fReport.state == SwitchedAway) return Fail(UnexpectedTransition);
		// HWSP pointer may precede the entry write. Caller retries this SAME
		// entry under a bounded deadline; never advance the CSB head on poison.
		if (raw == UINT64_MAX) return AwaitingEntryWrite;
		IntelN100CsbEvent e = intel_n100_csb_decode(raw);
		if (e.engineClass != fEngineClass || e.instance != 0) return Fail(WrongEngine);
		if ((e.toTag != 0x7ff && e.toTag != fReport.tag)
			|| (e.awayTag != 0x7ff && e.awayTag != fReport.tag))
			return Fail(WrongTag);
		// Idle-side counters and wait fields outside a wait are unspecified.
		// Preserve raw bits but do not infer meaning from those inactive fields.
		if ((e.toTag != 0x7ff && e.toCounter != 0)
			|| (e.awayTag != 0x7ff && e.awayCounter != 0))
			return Fail(WrongCounter);
		if (fReport.state == AwaitingActivation) {
			if (e.toTag != fReport.tag || e.awayTag != 0x7ff)
				return Fail(UnexpectedTransition);
			// With no outgoing context, switch detail is not a completion proof.
			fReport.state = Active;
		} else {
			if (e.toTag != 0x7ff || e.awayTag != fReport.tag
				|| e.newQueue || e.detail != 0)
				return Fail(UnexpectedTransition);
			fReport.state = SwitchedAway;
		}
		fReport.raw[fReport.accepted++] = raw;
		return Accepted;
	}
private:
	IntelN100ContextStatus(const IntelN100ContextStatus&) = delete;
	IntelN100ContextStatus& operator=(const IntelN100ContextStatus&) = delete;
	Result Fail(Error error)
	{
		fReport.error = error;
		fReport.state = Uncertain;
		return Rejected;
	}
	Report fReport;
	uint32_t fEngineClass;
};
#endif
