/* SPDX-License-Identifier: MIT */
/* Scoped drm_mm build: only interfaces in kpi_base.h are supported. */
#include <kpi_base.h>
#include <linux/gfp_types.h>
#include "../../upstream/uapi/linux/types.h"
#ifndef RENKU_BITMAP_TYPES_DEFINED
#define RENKU_BITMAP_TYPES_DEFINED
/* Linux bitmap layout, independent of native CPU-set or VM structures. */
#define DECLARE_BITMAP(name,bits) unsigned long name[BITS_TO_LONGS(bits)]
#ifdef RENKU_NATIVE
typedef __kernel_loff_t loff_t;
#endif
#endif

#ifndef RENKU_DMA_ADDR_T_DEFINED
#define RENKU_DMA_ADDR_T_DEFINED
typedef u64 dma_addr_t;
#endif
