/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_DMA_MAPPING_H
#define RENKU_LINUX_DMA_MAPPING_H
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/dma-direction.h>
#include <linux/errno.h>
struct device;
#ifndef DMA_BIT_MASK
#define DMA_BIT_MASK(n) ((n) == 64 ? ~0ULL : ((1ULL << (n)) - 1))
#endif
#define DMA_MAPPING_ERROR (~(dma_addr_t)0)
#define DMA_ATTR_NO_KERNEL_MAPPING (1UL << 4)
#define DMA_ATTR_SKIP_CPU_SYNC (1UL << 5)
#define DMA_ATTR_NO_WARN (1UL << 8)
int dma_set_mask_and_coherent(struct device *, u64);
void *dma_alloc_attrs(struct device *, size_t, dma_addr_t *, gfp_t, unsigned long);
void dma_free_attrs(struct device *, size_t, void *, dma_addr_t, unsigned long);
static inline void *dma_alloc_coherent(struct device *dev, size_t size,
	dma_addr_t *dma, gfp_t flags)
{ return dma_alloc_attrs(dev, size, dma, flags, 0); }
static inline void dma_free_coherent(struct device *dev, size_t size,
	void *cpu, dma_addr_t dma)
{ dma_free_attrs(dev, size, cpu, dma, 0); }
static inline int dma_mapping_error(struct device *dev, dma_addr_t address)
{ (void)dev; return address == DMA_MAPPING_ERROR ? -ENOMEM : 0; }
unsigned int dma_map_sg_attrs(struct device *, struct scatterlist *, int,
	enum dma_data_direction, unsigned long);
void dma_unmap_sg_attrs(struct device *, struct scatterlist *, int,
	enum dma_data_direction, unsigned long);
#define dma_map_sg(d,s,n,r) dma_map_sg_attrs(d,s,n,r,0)
#define dma_unmap_sg(d,s,n,r) dma_unmap_sg_attrs(d,s,n,r,0)
#endif
