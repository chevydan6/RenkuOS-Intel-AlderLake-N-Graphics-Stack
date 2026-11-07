// SPDX-License-Identifier: MIT
#pragma once
#include <Drivers.h>
#include <module.h>
#include "IntelVtdObservation.h"
#define VTD_MODULE "bus_managers/intel_vtd_observer/v0"
#define VTD_IOCTL (B_DEVICE_OP_CODES_END + 932)
struct VtdUnitResult {
	uint64 base;
	IntelVtdSample sample;
	status_t mapping,reading,cleanup;
	uint32 translation;
};
struct VtdResult {uint32 version;status_t status;uint32 count;VtdUnitResult units[16];};
struct VtdModule {module_info info;status_t (*observe)(VtdResult*);};
