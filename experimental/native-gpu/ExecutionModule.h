// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_EXECUTION_MODULE_H
#define NATIVE_GPU_EXECUTION_MODULE_H
#include <module.h>
#include "BufferCopy.h"
#include "LifecycleReport.h"
#define NATIVE_GPU_EXECUTION_MODULE "bus_managers/intel_memory_owner/native_execution/v3"
struct native_gpu_execution_module {
	module_info info;
	// Fixed sample plus synchronous private kernel staging interface. No kernel
	// pointer/callback escapes this call; only resident backing is GPU-visible.
	status_t (*run)(native_gpu_execution_report*);
	status_t (*copy)(const native_gpu_copy*,const uint32_t* source,
		uint32_t* destination,native_gpu_execution_report*);
	status_t (*status)(native_gpu_lifecycle_report*);
	void (*session_opened)();
	void (*session_closed)();
	status_t (*transition)(uint32_t action);
};
#endif
