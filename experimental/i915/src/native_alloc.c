/* SPDX-License-Identifier: MIT */
#include <linux/slab.h>
#include <linux/gfp.h>
#include <stdlib.h>
#ifndef RENKU_NATIVE
#error This adapter requires the native kernel
#endif
/* Existing native heap service; prototype in private/kernel/heap.h. */
extern void deferred_free(void *);
/* Exported native query from private/kernel/interrupts.h. */
extern bool interrupts_enabled(void);

void *kmalloc(size_t size, gfp_t flags)
{
	/* Initial scope: sleeping process-context allocations only. */
	if (!gfpflags_allow_blocking(flags) || (flags & ~(GFP_KERNEL | __GFP_ZERO))
		|| !interrupts_enabled()) return NULL;
	if (size == 0) return (void *)16; /* Linux ZERO_SIZE_PTR. */
	/* Native deferred_free embeds one list link in the freed object. */
	size = size < sizeof(void *) ? sizeof(void *) : size;
	return flags & __GFP_ZERO ? calloc(1, size) : malloc(size);
}

void kfree(const void *memory)
{
	if ((uintptr_t)memory <= 16) return;
	/* drm_vma_node_revoke frees its access record while holding a spinlock.
	 * Use the native deferred-free daemon instead of sleeping in that region. */
	if (interrupts_enabled()) free((void *)memory);
	else deferred_free((void *)memory);
}
