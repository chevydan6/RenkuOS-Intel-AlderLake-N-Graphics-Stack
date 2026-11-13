/* SPDX-License-Identifier: MIT */
#include <linux/dma-mapping.h>
#include "bus_dma_bridge.h"
/* Implement Linux's API; device addresses come only from native bus_dma. */
int dma_set_mask_and_coherent(struct device *device, u64 mask)
{
	return renku_bus_dma_set_mask(device, mask) == 0 ? 0 : -EIO;
}
void *dma_alloc_attrs(struct device *device, size_t size, dma_addr_t *dma,
	gfp_t flags, unsigned long attrs)
{
	/* The first coherent adapter can sleep. Do not pretend atomic allocation
	 * succeeds, or return a CPU pointer for NO_KERNEL_MAPPING semantics. */
	if (!(flags & __GFP_DIRECT_RECLAIM) || (flags & ~(GFP_KERNEL | __GFP_ZERO)) || attrs || !dma)
		return NULL;
	uint64_t address;
	void *cpu = renku_bus_dma_alloc(device, size, &address);
	if (cpu) *dma = address;
	return cpu;
}
void dma_free_attrs(struct device *device, size_t size, void *cpu,
	dma_addr_t dma, unsigned long attrs)
{
	if (attrs || renku_bus_dma_free(device, size, cpu, dma) != 0)
		WARN_ON_ONCE(true);
}

unsigned int dma_map_sg_attrs(struct device *device, struct scatterlist *sgl,
 int nents, enum dma_data_direction direction, unsigned long attrs)
{
 const unsigned long supported = DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_NO_KERNEL_MAPPING | DMA_ATTR_NO_WARN;
 if (!device || !sgl || nents <= 0 || !valid_dma_direction(direction) || (attrs & ~supported))
  return 0;
 struct scatterlist *sg = sgl;
 int done = 0;
 for (; done < nents; done++, sg = sg_next(sg)) {
  uint64_t address;
  if (!sg || sg_is_chain(sg) || !sg_page(sg) || !sg->length
   || !page_address(sg_page(sg))
   || renku_bus_dma_map(device, sg, sg_virt(sg), sg->length, direction,
    !!(attrs & DMA_ATTR_SKIP_CPU_SYNC), &address) != 0)
   break;
  sg_dma_address(sg) = address;
  sg_dma_len(sg) = sg->length;
 }
 if (done == nents) return nents;
 sg = sgl;
 for (int i = 0; i < done; i++, sg = sg_next(sg)) {
  if (renku_bus_dma_unmap(device, sg, sg->length, direction,
   !!(attrs & DMA_ATTR_SKIP_CPU_SYNC)) != 0) WARN_ON_ONCE(true);
  sg_dma_len(sg) = 0;
  sg_dma_address(sg) = DMA_MAPPING_ERROR;
 }
 return 0;
}
void dma_unmap_sg_attrs(struct device *device, struct scatterlist *sgl,
 int nents, enum dma_data_direction direction, unsigned long attrs)
{
 struct scatterlist *sg = sgl;
 if (!device || !sgl || nents <= 0 || !valid_dma_direction(direction)) { WARN_ON_ONCE(true); return; }
 for (int i = 0; i < nents; i++, sg = sg_next(sg)) {
  if (!sg || renku_bus_dma_unmap(device, sg, sg->length, direction,
   !!(attrs & DMA_ATTR_SKIP_CPU_SYNC)) != 0) { WARN_ON_ONCE(true); return; }
  sg_dma_len(sg) = 0;
  sg_dma_address(sg) = DMA_MAPPING_ERROR;
 }
}
