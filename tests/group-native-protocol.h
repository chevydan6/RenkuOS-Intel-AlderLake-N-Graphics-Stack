// SPDX-License-Identifier: MIT
#pragma once
#include <Drivers.h>
#include "IntelN100Forcewake.h"
#include "IntelN100ForcewakeGroup.h"
#define GROUP_HANDSHAKE_IOCTL (B_DEVICE_OP_CODES_END + 938)
struct GroupHandshakeResult {
	uint32 version;
	status_t module, transaction;
	IntelN100Forcewake::Report bootstrap;
	IntelN100ForcewakeGroup::Report group;
	uint32 rawFuse;
};
