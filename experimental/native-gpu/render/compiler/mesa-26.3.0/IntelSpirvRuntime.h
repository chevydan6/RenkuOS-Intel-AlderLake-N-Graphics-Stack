/* SPDX-License-Identifier: MIT
 * Copyright 2026 Native GPU validation contributors
 */
#ifndef RENKU_INTEL_SPIRV_RUNTIME_H
#define RENKU_INTEL_SPIRV_RUNTIME_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define RENKU_INTEL_COMPILER_ABI 1u
enum {
 RENKU_INTEL_COMPILE_OK=0,
 RENKU_INTEL_COMPILE_INVALID=-1,
 RENKU_INTEL_COMPILE_OUT_OF_MEMORY=-2,
 RENKU_INTEL_COMPILE_WRONG_DEVICE=-3,
 RENKU_INTEL_COMPILE_FAILED=-4,
 RENKU_INTEL_COMPILE_UNSUPPORTED=-5,
 RENKU_INTEL_COMPILE_OUTPUT_TOO_SMALL=-6
};

typedef struct {
 void* code;
 size_t capacity;
 size_t bytes;
 uint32_t grf_used, dispatch_grf_start_reg;
 uint32_t total_scratch, num_relocs;
 uint32_t urb_read_length, urb_entry_size, dispatch_mode;
 uint32_t dispatch_8, dispatch_16, dispatch_32;
 uint32_t prog_offset_16, prog_offset_32;
 uint32_t dispatch_grf_start_reg_16, dispatch_grf_start_reg_32;
} RenkuIntelStage;

typedef struct {
 uint32_t abi_version;
 RenkuIntelStage vertex, fragment;
} RenkuIntelCompilePair;

/* Input sizes are in 32-bit words. Caller supplies valid Vulkan 1.0 SPIR-V;
 * this small adapter checks the header and bounds, not every SPIR-V rule.
 * Caller owns the output code buffers.
 * Returns 0 on success; a negative status on invalid input, compilation
 * failure, unsupported hardware profile, or insufficient output capacity.
 */
int renku_intel_compile_vulkan_pair(const uint32_t* vertex, size_t vertex_words,
 const uint32_t* fragment, size_t fragment_words,
 RenkuIntelCompilePair* output, char* error, size_t error_capacity);

#ifdef __cplusplus
}
#endif
#endif
