// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RESOURCE_MODULE_H
#define NATIVE_GPU_RESOURCE_MODULE_H
#include <module.h>
#include "ResourceCommand.h"
#define NATIVE_GPU_RESOURCE_MODULE "bus_managers/intel_memory_owner/native_resource/v1"
struct native_gpu_resource_module {
 module_info info;
 status_t (*open)(uint64_t* session);
 status_t (*close)(uint64_t session);
 status_t (*create)(uint64_t session,uint64_t bytes,uint32_t* handle,
  uint64_t* gpu,uint64_t* actual);
 status_t (*transfer)(uint64_t session,bool write,uint32_t handle,
  uint64_t offset,uint8_t* bytes,uint32_t count);
 status_t (*destroy)(uint64_t session,uint32_t handle);
};
#endif
