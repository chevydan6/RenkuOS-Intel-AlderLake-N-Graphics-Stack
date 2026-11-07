// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_CONTEXT_IMAGE_H
#define INTEL_N100_CONTEXT_IMAGE_H
#include "IntelN100Ppgtt.h"
#include <string.h>

// CPU-only initial Gen12 BCS0 image for N100 revision0, primary GT (GSI=0),
// four-level PPGTT, no default saved context. Does not own/pin/validate mappings,
// program PAT/MOCS, publish commands or authorize GPU execution.
// Field/source evidence: research/n100-submission/CONTEXT-IMAGE-DESIGN.md.
class IntelN100ContextImage {
public:
	struct Input {
		uint32_t identity, revision;
		uint64_t contextGgtt, ringGgtt, pml4Dma;
		uint32_t ringBytes, tail;
	};
	static constexpr size_t kBytes = 16384;
	static bool Build(const Input& input, uint32_t* output, size_t bytes)
	{
		// Snapshot before clearing output, even if caller stores input there.
		Input in = input;
		uintptr_t cpu = reinterpret_cast<uintptr_t>(output);
		if (!output || bytes != kBytes || (cpu & 3) || cpu > UINTPTR_MAX - (kBytes - 1)
			|| in.identity != 0x46d18086 || in.revision != 0
			|| (in.contextGgtt & 4095) || in.contextGgtt > (1ULL << 32) - kBytes
			|| in.ringBytes < 4096 || in.ringBytes > (1U << 21)
			|| (in.ringBytes & (in.ringBytes - 1))
			|| (in.ringGgtt & 4095) || in.ringGgtt > (1ULL << 32) - in.ringBytes
			|| (in.contextGgtt < in.ringGgtt + in.ringBytes
				&& in.ringGgtt < in.contextGgtt + kBytes)
			|| !intel_n100_ppgtt_page(in.pml4Dma)
			|| !in.tail || (in.tail & 7) || in.tail > in.ringBytes - 64)
			return false;
		// All fallible validation precedes writes. Fresh ppHWSP, register page,
		// indirect-workaround page and per-context batch page, each4KiB.
		memset(output, 0, kBytes);
		uint32_t* regs = output + 1024;
		static const uint32_t first[] = {0x244, 0x034, 0x030, 0x038, 0x03c,
			0x168, 0x140, 0x110, 0x1c0, 0x1c4, 0x1c8, 0x180, 0x2b4};
		static const uint32_t second[] = {0x3a8, 0x28c, 0x288, 0x284, 0x280,
			0x27c, 0x278, 0x274, 0x270};
		regs[1] = 0x11081019; // LRI13, force-posted, CS-relative MMIO.
		for (unsigned i = 0; i < 13; i++) regs[2 + 2 * i] = 0x22000 + first[i];
		regs[0x21] = 0x11081011; // Five NOOP slots, then LRI9.
		for (unsigned i = 0; i < 9; i++) regs[0x22 + 2 * i] = 0x22000 + second[i];
		regs[0x34] = 0x05000001; // Initial context-image BBE, Gen11+ bit0.
		regs[3] = 0x00090009; // Masked inhibit synchronous switch + restore inhibit.
		regs[5] = 0; // Fresh ring head.
		regs[7] = in.tail;
		regs[9] = uint32_t(in.ringGgtt);
		regs[11] = (in.ringBytes - 4096) | 1;
		regs[0x31] = uint32_t(in.pml4Dma >> 32);
		regs[0x33] = uint32_t(in.pml4Dma);
		regs[0x61] = 0x01000000; // Saved MI_MODE: masked STOP_RING(bit8) clear.

		uint32_t context = uint32_t(in.contextGgtt);
		uint32_t indirect = context + 8192;
		regs[0x13] = (context + 12288) | 5; // PER_CTX_BB force + valid.
		regs[0x15] = indirect | 2; // Two cachelines (128 bytes).
		regs[0x17] = 0x0d << 6; // Gen12 indirect-context restore offset.
		uint32_t* wa = output + 2048;
		const uint32_t commands[] = {
			0x14c80002, 0x600, context + 4096 + 0x23 * 4, 0,
			0x150c0001, 0x600, 0x3a8,
			0x150c0001, 0x600, 0x3a8,
			0x14c80002, 0x600, context + 4096 + 0x75 * 4, 0,
			// N100 has no flat CCS: AUX invalidation + register-poll wait.
			0x11020001, 0x4248, 1,
			0x0e01c003, 0, 0x4248, 0, 0
		};
		memcpy(wa, commands, sizeof(commands)); // Remaining128-byte span is NOOP.
		// Pinned source populates this separate predicate helper unconditionally;
		// it is not inside the128-byte indirect span or linked by this builder.
		const uint32_t predicate[] = {
			0x10400002, indirect + 4096 - 8, 0, 0,
			0x05008000, 0x00800000,
			0x10400002, indirect + 4096 - 8, 0, 1, 0x05000000
		};
		memcpy(wa + 512, predicate, sizeof(predicate));
		output[3072] = 0x05000000; // Separate PER_CTX_BB must terminate itself.
		return true;
	}
};
#endif
