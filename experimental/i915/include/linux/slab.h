/* SPDX-License-Identifier: MIT */
/* Scoped drm_mm build: only interfaces in kpi_base.h are supported. */
#include <kpi_base.h>
#include <linux/gfp_types.h>
#include <linux/errno.h>
void *kmalloc(size_t, gfp_t);
void kfree(const void *);
