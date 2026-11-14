// SPDX-License-Identifier: MIT
// Copyright © 2014 Intel Corporation
// Native integration Copyright (c) 2026 chevydan6
#ifndef NATIVE_GPU_BCS_ENGINE_H
#define NATIVE_GPU_BCS_ENGINE_H
#include <IntelN100ContextStatus.h>

namespace NativeGpu {
// Fixed Gen12LP BCS0 setup and ELSQ publication, adapted from Linux v6.6
// reset_csb_pointers(), enable_execlists(), write_desc() and
// execlists_submit_ports(). No Linux scheduler/device objects are required.
//
// The retained native execution owner calls this AFTER its BCS reset and
// PAT/MOCS/TLB/DMA admission, while holding the complete forcewake group.
// IO.safe must revalidate that same owner/device/power/memory contract. The
// HWSP is the already initialized, exclusively owned page used by RequestImage.
// IO.visible orders CPU request/context/table stores and establishes visibility
// for the selected mappings; a compiler barrier alone is not sufficient.
//
// This helper neither acquires ownership nor retires it. After any attempted
// register store, retain the HWSP/mappings and power on failure. In particular,
// loadAttempted is set BEFORE issuing LOAD, including ambiguous write failures.
// Published means only that LOAD was issued; it does not prove GPU completion.
// Serialize calls and retain this ledger through completion/reset/reboot.
class BcsEngine {
public:
	enum Stage : uint32_t { Empty, Initializing, Initialized, Publishing, Published, Failed };
	enum Error : uint32_t { None, InvalidInput, Unsafe, Clock, Deadline,
		ReadFailed, WriteFailed, VisibilityFailed, UnexpectedRegister };
	struct IO {
		void* context;
		uint64_t (*now)(void*);
		bool (*safe)(void*);
		bool (*read)(void*, uint32_t offset, uint32_t& value);
		bool (*write)(void*, uint32_t offset, uint32_t value);
		bool (*visible)(void*);
	};
	struct Report {
		Stage stage;
		Error error;
		uint32_t reads, writes, lastOffset, lastValue;
		uint32_t attemptedOffset, attemptedValue, engineTouched, loadAttempted;
		uint64_t start, previous, deadline;
	};
	BcsEngine() : fReport() {}
	const Report& LastReport() const { return fReport; }
	bool Initialize(const IO& io, uint32_t identity, uint32_t revision,
		uint64_t statusGgtt, uint64_t deadline, uint32_t initialCsb = 11)
	{
		if (fReport.stage != Empty) return false;
		if (!Valid(io) || identity != 0x46d18086 || revision != 0
			|| (statusGgtt & 4095) || statusGgtt > (1ULL << 32) - 4096 || initialCsb >= 12)
			return Fail(InvalidInput);
		fReport.stage = Initializing;
		fReport.start = fReport.previous = io.now(io.context);
		fReport.deadline = deadline;
		if (fReport.start == UINT64_MAX || deadline <= fReport.start
			|| deadline - fReport.start > 100000) return Fail(InvalidInput);
		uint32_t value;
		if (!Read(io, 0x2203c, value)) return false; // RING_CTL, not active.
		if (value & 1) return Fail(UnexpectedRegister);
		if (!Read(io, 0x2209c, value)) return false; // RING_MI_MODE idle.
		if (!(value & 0x200)) return Fail(UnexpectedRegister);
		if (!Visible(io)) return false;
		// First CSB cursor reset; HWSP memory must use this same initial index.
		const uint32_t cursor = initialCsb | (initialCsb << 8);
		if (!Store(io, 0x223a0, 0xffff0000 | cursor) || !Read(io, 0x223a0, value)) return false;
		if ((value & 0xffff) != cursor) return Fail(UnexpectedRegister);
		// HWSTAM then Gen11+ disable-legacy-mode, retaining reset's prefetch
		// disable bit. Clear STOP_RING only with the engine in execlists mode.
		if (!Store(io, 0x22098, UINT32_MAX)
			|| !Store(io, 0x2229c, 0x00080008)
			|| !Store(io, 0x2209c, 0x01000000)
			|| !Store(io, 0x22080, uint32_t(statusGgtt))
			|| !Read(io, 0x22080, value)) return false;
		if (value != statusGgtt) return Fail(UnexpectedRegister);
		// Polling owner has no IRQ integration. Unlike Linux's interrupt path,
		// keep engine error interrupts masked; errors must be polled by owner.
		if (!Store(io, 0x220b4, UINT32_MAX) || !Visible(io)
			|| !Store(io, 0x223a0, 0xffff0000 | cursor)
			|| !Read(io, 0x223a0, value)) return false;
		if ((value & 0xffff) != cursor) return Fail(UnexpectedRegister);
		fReport.stage = Initialized;
		return true;
	}
	bool Publish(const IO& io, uint64_t descriptor)
	{
		if (fReport.stage != Initialized) return false;
		uint64_t expected = 0;
		if (!Valid(io) || !intel_n100_bcs_descriptor(0x46d18086, 0,
			descriptor & 0xfffff000ULL, uint32_t((descriptor >> 37) & 0x7ff), &expected)
			|| descriptor != expected) return Fail(InvalidInput);
		fReport.stage = Publishing;
		if (!Visible(io)) return false;
		// Two ports, descending order. ELSQ contents persist after LOAD;
		// always clear unused port1, then port0 low/high (not ELSP high/low).
		if (!Store(io, 0x22518, 0) || !Store(io, 0x2251c, 0)
			|| !Store(io, 0x22510, uint32_t(descriptor))
			|| !Store(io, 0x22514, uint32_t(descriptor >> 32))
			|| !Store(io, 0x22550, 1)) return false;
		fReport.stage = Published;
		return true;
	}
private:
	static bool Valid(const IO& io)
	{ return io.now && io.safe && io.read && io.write && io.visible; }
	bool Fail(Error error) { fReport.stage = Failed; fReport.error = error; return false; }
	bool Check(const IO& io)
	{
		uint64_t now = io.now(io.context);
		if (now == UINT64_MAX || now < fReport.previous) return Fail(Clock);
		fReport.previous = now;
		if (now >= fReport.deadline) return Fail(Deadline);
		if (!io.safe(io.context)) return Fail(Unsafe);
		now = io.now(io.context);
		if (now == UINT64_MAX || now < fReport.previous) return Fail(Clock);
		fReport.previous = now;
		return now < fReport.deadline || Fail(Deadline);
	}
	bool Read(const IO& io, uint32_t offset, uint32_t& value)
	{
		if (!Check(io)) return false;
		fReport.reads++;
		fReport.lastOffset = offset;
		if (!io.read(io.context, offset, value)) return Fail(ReadFailed);
		fReport.lastValue = value;
		if (value == UINT32_MAX) return Fail(ReadFailed);
		return Check(io);
	}
	bool Store(const IO& io, uint32_t offset, uint32_t value)
	{
		if (!Check(io)) return false;
		fReport.attemptedOffset = offset;
		fReport.attemptedValue = value;
		fReport.engineTouched = 1;
		if (offset == 0x22550) fReport.loadAttempted = 1;
		fReport.writes++;
		if (!io.write(io.context, offset, value)) return Fail(WriteFailed);
		return Check(io);
	}
	bool Visible(const IO& io)
	{
		if (!Check(io)) return false;
		if (!io.visible(io.context)) return Fail(VisibilityFailed);
		return Check(io);
	}
	BcsEngine(const BcsEngine&) = delete;
	BcsEngine& operator=(const BcsEngine&) = delete;
	Report fReport;
};
}
#endif
