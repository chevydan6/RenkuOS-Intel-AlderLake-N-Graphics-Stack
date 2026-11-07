// SPDX-License-Identifier: MIT
#pragma once
#include <Drivers.h>
#include <module.h>
#define DMAR_MODULE "bus_managers/intel_dmar_snapshot/v0"
#define DMAR_IOCTL (B_DEVICE_OP_CODES_END + 931)
struct DmarSnapshot {uint32 version;status_t status;uint32 length;uint8 bytes[4096];};
struct DmarModule {module_info info;status_t (*snapshot)(DmarSnapshot*);};
