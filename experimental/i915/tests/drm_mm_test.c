/* SPDX-License-Identifier: MIT */
/* Real upstream CPU allocator tests. No GPU, DMA or mocked allocator. */
#include <drm/drm_mm.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool renku_kpi_warn(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	abort();
}

static unsigned int state = 0x91501234;
static unsigned int next_random(void)
{
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	return state;
}

static void verify(struct drm_mm_node *nodes, unsigned int count, u64 base, u64 end)
{
	for (unsigned int i = 0; i < count; ++i) {
		if (!drm_mm_node_allocated(nodes + i)) continue;
		assert(nodes[i].start >= base && nodes[i].start < end);
		assert(nodes[i].size <= end - nodes[i].start);
		for (unsigned int j = i + 1; j < count; ++j) {
			if (!drm_mm_node_allocated(nodes + j)) continue;
			assert(nodes[i].start + nodes[i].size <= nodes[j].start ||
				nodes[j].start + nodes[j].size <= nodes[i].start);
		}
	}
}

int main(void)
{
	struct drm_mm mm;
	struct drm_mm_node nodes[64] = {0};
	const u64 base = 0x100000000ULL, length = 256 * 4096;
	drm_mm_init(&mm, base, length);
	/* Preserve an existing firmware reservation and reject an overlap. */
	nodes[0].start = base + 32 * 4096;
	nodes[0].size = 16 * 4096;
	assert(drm_mm_reserve_node(&mm, nodes) == 0);
	nodes[1].start = nodes[0].start;
	nodes[1].size = 4096;
	assert(drm_mm_reserve_node(&mm, nodes + 1) == -ENOSPC);
	memset(nodes + 1, 0, sizeof(nodes[1]));
	unsigned int allocations = 0, full = 0;
	for (unsigned int step = 0; step < 20000; ++step) {
		unsigned int index = 1 + next_random() % 63;
		struct drm_mm_node *node = nodes + index;
		if (drm_mm_node_allocated(node)) {
			drm_mm_remove_node(node);
			memset(node, 0, sizeof(*node));
		} else {
			u64 size = (1 + next_random() % 24) * 4096;
			/* Exercise both power-of-two and general division alignment. */
			u64 align = (1 + next_random() % 5) * 4096;
			enum drm_mm_insert_mode mode = next_random() % 3;
			int error = drm_mm_insert_node_in_range(&mm, node, size, align, 0,
				base, base + length, mode);
			assert(error == 0 || error == -ENOSPC);
			if (error == 0) {
				assert(node->start % align == 0);
				++allocations;
			} else {
				assert(!drm_mm_node_allocated(node));
				++full;
			}
		}
		verify(nodes, 64, base, base + length);
	}
	for (unsigned int i = 0; i < 64; ++i)
		if (drm_mm_node_allocated(nodes + i)) drm_mm_remove_node(nodes + i);
	assert(drm_mm_clean(&mm));
	struct drm_mm_node all = {0};
	assert(drm_mm_insert_node(&mm, &all, length) == 0);
	assert(all.start == base);
	drm_mm_remove_node(&all);
	drm_mm_takedown(&mm);
	printf("CPU allocator: 20000 transitions; %u allocations, %u full; clean teardown\n",
		allocations, full);
	return 0;
}
