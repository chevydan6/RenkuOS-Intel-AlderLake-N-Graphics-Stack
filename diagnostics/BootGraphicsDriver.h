// SPDX-License-Identifier: MIT
#ifndef GPU_BOOT_GRAPHICS_DRIVER_H
#define GPU_BOOT_GRAPHICS_DRIVER_H

#include <Drivers.h>
#include "BootFrameBufferInfo.h"

#define BOOT_GRAPHICS_DEVICE "misc/boot_graphics"
#define BOOT_GRAPHICS_DEVICE_PATH "/dev/" BOOT_GRAPHICS_DEVICE

enum {
	BOOT_GRAPHICS_GET_BOOT_INFO = B_DEVICE_OP_CODES_END + 1
};

#endif
