// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RENDER_MODULE_H
#define NATIVE_GPU_RENDER_MODULE_H
#include <module.h>
#include "RenderReport.h"
#define NATIVE_GPU_RENDER_MODULE "bus_managers/intel_memory_owner/native_render/v1"
#define NATIVE_GPU_RENDER_PROFILE_N100_DISPLAY_ONLY_V1 UINT32_C(1)
struct native_gpu_render_module {
 module_info info;
 status_t (*run)(native_gpu_render_command*);
 // Trusted graphics-driver call only, after finish_display(lease,true).
 status_t (*acknowledge_display_profile)(uint64_t displayLease,uint32_t profile);
};
#endif
