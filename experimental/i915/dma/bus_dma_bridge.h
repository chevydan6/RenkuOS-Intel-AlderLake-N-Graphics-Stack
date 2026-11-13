/* SPDX-License-Identifier: MIT */
#ifndef RENKU_BUS_DMA_BRIDGE_H
#define RENKU_BUS_DMA_BRIDGE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Separate translation units keep Linux and FreeBSD/native types/error domains apart. */
int renku_bus_dma_init(void);
int renku_bus_dma_uninit(void);
int renku_bus_dma_set_mask(const void *device, uint64_t mask);
int renku_bus_dma_forget(const void *device);
void *renku_bus_dma_alloc(const void *device, size_t size, uint64_t *dma);
int renku_bus_dma_free(const void *device, size_t size, void *cpu, uint64_t dma);
int renku_bus_dma_map(const void *device, const void *key, void *cpu, size_t size,
 int direction, int skip_sync, uint64_t *dma);
int renku_bus_dma_unmap(const void *device, const void *key, size_t size,
 int direction, int skip_sync);
#ifdef __cplusplus
}
#endif
#endif
