/* SPDX-License-Identifier: MIT */
#ifndef RENKU_RING_PORT_H
#define RENKU_RING_PORT_H
#include <kpi_base.h>
#include <linux/err.h>
#define CACHELINE_BYTES 64u
#define GEM_BUG_ON(x) do { if (x) panic("i915 ring assertion: %s", #x); } while (0)
#define IS_ALIGNED(x, a) (((x) & ((a) - 1)) == 0)
#define CONFIG_DRM_I915_DEBUG_GEM 0
#define POISON_INUSE 0x5a5a5a5au
/* Private slice layouts, NOT the complete Linux object layouts or ABI. */
struct intel_ring {
	u32 head, tail, emit, size, effective_size, space;
	void *vaddr;
};
struct i915_request {
	struct intel_ring *ring;
	unsigned int reserved_space;
	bool nopreempt; /* Native slice's software request attribute. */
};
struct intel_timeline;
extern struct intel_timeline *i915_request_timeline(struct i915_request *);
/* Deliberately unresolved: must wait/retire real GPU requests before reuse. */
extern int renku_ring_wait_for_space(struct intel_ring *,
	struct intel_timeline *, unsigned int);
#define wait_for_space renku_ring_wait_for_space
static inline void *memset64(void *p, u64 value, size_t count)
{
	u64 *out = p;
	for (size_t i = 0; i < count; ++i) out[i] = value;
	return p;
}
static inline void *memset32(void *p, u32 value, size_t count)
{
	u32 *out = p;
	for (size_t i = 0; i < count; ++i) out[i] = value;
	return p;
}
static inline u32 intel_ring_wrap(const struct intel_ring *ring, u32 pos)
{
	return pos & (ring->size - 1);
}
static inline unsigned int __intel_ring_space(unsigned int head,
	unsigned int tail, unsigned int size)
{
	GEM_BUG_ON(!is_power_of_2(size));
	return (head - tail - CACHELINE_BYTES) & (size - 1);
}
unsigned int intel_ring_update_space(struct intel_ring *);
void intel_ring_reset(struct intel_ring *, u32);
u32 *intel_ring_begin(struct i915_request *, unsigned int);
#endif
