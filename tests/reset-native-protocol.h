// SPDX-License-Identifier: MIT
#pragma once
#include <Drivers.h>
#include "IntelN100Forcewake.h"
#include "IntelN100ForcewakeGroup.h"
#include "IntelN100BcsReset.h"
#include "IntelN100ResetSafety.h"
#define BCS_RESET_IOCTL (B_DEVICE_OP_CODES_END + 939)
struct BcsResetResult {
	uint32 version;
	status_t module, transaction;
	IntelN100Forcewake::Report bootstrap;
	IntelN100ForcewakeGroup::Report group;
	IntelN100BcsReset::Report reset;
	uint32 rawFuse;
	status_t safetyModule, safetyRead;
	IntelN100ResetSafety safety;
};
