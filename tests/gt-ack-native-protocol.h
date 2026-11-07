// SPDX-License-Identifier: MIT
#pragma once
#include <Drivers.h>
#include "IntelN100GtAck.h"
#define GT_ACK_IOCTL (B_DEVICE_OP_CODES_END + 934)
struct GtAckResult {
	uint32 version;
	status_t module, observation;
	IntelN100GtAckSample sample;
};
