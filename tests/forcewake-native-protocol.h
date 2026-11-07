// SPDX-License-Identifier: MIT
#pragma once
#include <Drivers.h>
#include "IntelN100Forcewake.h"
#define FORCEWAKE_IOCTL (B_DEVICE_OP_CODES_END + 935)
struct ForcewakeResult {
	uint32 version;
	status_t module, transaction;
	IntelN100Forcewake::Report report;
};
