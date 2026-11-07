// SPDX-License-Identifier: MIT
#pragma once
#include <Drivers.h>
#include "IntelN100Forcewake.h"
#include "IntelN100BcsInventory.h"
#define BCS_INVENTORY_IOCTL (B_DEVICE_OP_CODES_END + 936)
struct BcsInventoryResult {
	uint32 version;
	status_t module, transaction;
	IntelN100Forcewake::Report report;
	IntelN100BcsInventory registers;
};
