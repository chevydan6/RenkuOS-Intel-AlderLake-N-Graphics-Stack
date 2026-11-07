// SPDX-License-Identifier: MIT
#pragma once
#include <Drivers.h>
#include "IntelN100PciPower.h"
#define PCI_OWNER_IOCTL (B_DEVICE_OP_CODES_END + 933)
struct PciOwnerRun {
	uint64 lease;
	status_t begin, duplicate, end, stale;
	IntelN100PciSnapshot snapshot;
};
struct PciOwnerResult {
	uint32 version;
	status_t module;
	uint32 completed;
	PciOwnerRun runs[2];
};
