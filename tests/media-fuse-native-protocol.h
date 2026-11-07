// SPDX-License-Identifier: MIT
#pragma once
#include <Drivers.h>
#include "IntelN100Forcewake.h"
#define MEDIA_FUSE_IOCTL (B_DEVICE_OP_CODES_END + 937)
struct MediaFuseResult {
	uint32 version;
	status_t module, transaction;
	IntelN100Forcewake::Report report;
	uint32 rawFuse;
};
