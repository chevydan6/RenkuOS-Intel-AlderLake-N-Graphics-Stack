/* SPDX-License-Identifier: MIT */
/* Original Linux command construction only; no MMIO/DMA/GPU is simulated. */
#include <emitter_port.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void panic(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	abort();
}
/* A real request timeline/retirement backend is deliberately absent. These
 * fail-fast test entry points must never be reached by CPU-only reservations. */
struct intel_timeline *i915_request_timeline(struct i915_request *rq)
{ (void)rq; abort(); }
int renku_ring_wait_for_space(struct intel_ring *r, struct intel_timeline *t, unsigned int n)
{ (void)r; (void)t; (void)n; abort(); }

struct buffers { u32 before[16], words[128], after[16]; };
int main(void)
{
	unsigned int cases = 0;
	for (unsigned int direct = 0; direct < 2; ++direct)
	for (unsigned int nopreempt = 0; nopreempt < 2; ++nopreempt)
	for (unsigned int secure = 0; secure < 2; ++secure)
	for (unsigned int wrap = 0; wrap < 2; ++wrap) {
		_Alignas(64) struct buffers b;
		for (unsigned int i = 0; i < 16; ++i) b.before[i] = b.after[i] = 0xfeedbeef;
		for (unsigned int i = 0; i < 128; ++i) b.words[i] = 0xa5a5a5a5;
		struct intel_ring ring = { .size = 512, .effective_size = 512, .vaddr = b.words };
		unsigned int start = wrap ? 504 : 0;
		intel_ring_reset(&ring, start);
		struct i915_request rq = { .ring = &ring, .reserved_space = 32, .nopreempt = nopreempt };
		const u64 offset = 0x0000001234567000ULL;
		int error = direct ? gen8_emit_bb_start_noarb(&rq, offset, 4096, secure) :
			gen8_emit_bb_start(&rq, offset, 4096, secure);
		assert(error == 0);
		bool noarb = direct || nopreempt;
		unsigned int count = noarb ? 4 : 6;
		/* Literal expected dwords: v6.6 MI_ARB_ON_OFF and MI_BATCH_BUFFER_START,
		 * with distinct high address bits and source's PPGTT selector. */
		u32 expected[6] = { noarb ? 0x04000000u : 0x04000001u,
			secure ? 0x18800001u : 0x18800101u,
			0x34567000u, 0x12u, 0x04000000u, 0 };
		for (unsigned int i = 0; i < count; ++i) assert(b.words[i] == expected[i]);
		for (unsigned int i = count; i < 128; ++i)
			assert(b.words[i] == (wrap && i >= 126 ? 0u : 0xa5a5a5a5u));
		for (unsigned int i = 0; i < 16; ++i)
			assert(b.before[i] == 0xfeedbeef && b.after[i] == 0xfeedbeef);
		assert(ring.emit == count * 4 && ring.head == start && ring.tail == start);
		assert(ring.space == 448 - count * 4 - (wrap ? 8 : 0));
		++cases;
	}
	assert(gen8_canonical_addr(0x800000000000ULL) == 0xffff800000000000ULL);
	assert(gen8_noncanonical_addr(0xffff800000000000ULL) == 0x800000000000ULL);
	printf("CPU batch-start encoding: %u branch/wrap cases, 64-bit addresses and guards; pass\n", cases);
	puts("No GPU command was submitted or completed.");
	return 0;
}
