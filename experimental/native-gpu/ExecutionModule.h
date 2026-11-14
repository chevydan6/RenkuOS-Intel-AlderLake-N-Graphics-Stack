// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_EXECUTION_MODULE_H
#define NATIVE_GPU_EXECUTION_MODULE_H
#include <module.h>
#include "ExecutionReport.h"
#define NATIVE_GPU_EXECUTION_MODULE "bus_managers/intel_memory_owner/native_execution/v1"
struct native_gpu_execution_module {
	module_info info;
	// Fixed kernel-built nonce plus 4KiB copy. One attempt per boot. No user
	// commands, addresses, buffers or callback inputs. Resident owner keeps
	// backing after hardware access, even on success, until controlled reboot.
	status_t (*run)(native_gpu_execution_report*);
};
#endif
