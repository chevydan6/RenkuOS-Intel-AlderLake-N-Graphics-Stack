/* SPDX-License-Identifier: MIT */
#ifndef RENKU_EMITTER_PORT_H
#define RENKU_EMITTER_PORT_H
#include <ring_port.h>
#include <gt/intel_gpu_commands.h>
/* Linux v6.6 gt/intel_engine_types.h; internal trusted dispatch only. */
#define I915_DISPATCH_SECURE BIT(0)
#define upper_32_bits(value) ((u32)((u64)(value) >> 32))
static inline bool i915_request_has_nopreempt(const struct i915_request *rq)
{
	return unlikely(rq->nopreempt);
}
int gen8_emit_bb_start_noarb(struct i915_request *, u64, u32, unsigned int);
int gen8_emit_bb_start(struct i915_request *, u64, u32, unsigned int);
#endif
