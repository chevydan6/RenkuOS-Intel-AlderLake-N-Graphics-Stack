/* SPDX-License-Identifier: MIT */
#ifndef I915_LINUX_DMA_TEST_H
#define I915_LINUX_DMA_TEST_H
#include <SupportDefs.h>
#define LINUX_DMA_IOCTL 0x9150444d
struct linux_dma_result {
	uint32 version, checks, failures, coherent_allocations, coherent_frees;
	uint32 streaming_maps, streaming_unmaps;
};
#endif
