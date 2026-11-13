/* SPDX-License-Identifier: MIT */
/* Scoped drm_mm build: only interfaces in kpi_base.h are supported. */
#include <kpi_base.h>
#include <linux/gfp_types.h>
#include "../../upstream/uapi/linux/types.h"

#ifndef RENKU_DMA_ADDR_T_DEFINED
#define RENKU_DMA_ADDR_T_DEFINED
typedef u64 dma_addr_t;
#endif
