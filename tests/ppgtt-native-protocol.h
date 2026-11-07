// SPDX-License-Identifier: MIT
#pragma once
#include <Drivers.h>
#include <module.h>
#define PPGTT_TEST_MODULE "bus_managers/intel_ppgtt_test/v0"
#define PPGTT_TEST_IOCTL (B_DEVICE_OP_CODES_END + 930)
struct PpgttTestResult {
	uint32 version;
	status_t status;
	uint32 completed, stage, current_case, verified_areas, absent_areas;
	uint32 tree_state, live_areas;
	status_t provider_error;
};
struct PpgttTestModule {
	module_info info;
	status_t (*run)(PpgttTestResult*);
};
