// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_PREPARE_MODULE_H
#define NATIVE_GPU_PREPARE_MODULE_H
#include <module.h>
#include "PrepareReport.h"
#define NATIVE_GPU_PREPARE_MODULE "bus_managers/intel_native_prepare/v1"
struct native_gpu_prepare_module {
	module_info info;
	status_t (*prepare)(native_gpu_prepare_report*);
};
#endif
