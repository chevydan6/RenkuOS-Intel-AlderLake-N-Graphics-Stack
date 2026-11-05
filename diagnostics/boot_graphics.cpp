// SPDX-License-Identifier: MIT
// Private diagnostic: exposes immutable boot metadata, never GPU mappings.

#include <KernelExport.h>
#include <boot_item.h>
#include <frame_buffer_console.h>
#include <kernel.h>

#include <string.h>
#include <unistd.h>

#include "BootGraphicsDriver.h"

static BootFrameBufferInfo sBootInfo;
static const char* sDevices[] = { BOOT_GRAPHICS_DEVICE, NULL };

int32 api_version = B_CUR_DRIVER_API_VERSION;


static status_t
DeviceOpen(const char*, uint32, void** cookie)
{
	if (geteuid() != 0)
		return B_NOT_ALLOWED;
	*cookie = NULL;
	return B_OK;
}


static status_t
DeviceClose(void*)
{
	return B_OK;
}


static status_t
DeviceControl(void*, uint32 operation, void* buffer, size_t size)
{
	if (geteuid() != 0)
		return B_NOT_ALLOWED;
	if (operation != BOOT_GRAPHICS_GET_BOOT_INFO)
		return B_DEV_INVALID_IOCTL;
	if (size != sizeof(sBootInfo))
		return B_BAD_VALUE;
	if (!is_user_address_range(buffer, size))
		return B_BAD_ADDRESS;
	return user_memcpy(buffer, &sBootInfo, sizeof(sBootInfo));
}


static status_t
DeviceRead(void*, off_t, void*, size_t* length)
{
	*length = 0;
	return B_NOT_ALLOWED;
}


static status_t
DeviceWrite(void*, off_t, const void*, size_t* length)
{
	*length = 0;
	return B_NOT_ALLOWED;
}


static device_hooks sHooks = {
	DeviceOpen, DeviceClose, DeviceClose, DeviceControl, DeviceRead, DeviceWrite,
	NULL, NULL, NULL, NULL
};


status_t
init_hardware()
{
	return B_OK;
}


status_t
init_driver()
{
	size_t size = 0;
	const frame_buffer_boot_info* info
		= static_cast<const frame_buffer_boot_info*>(
			get_boot_item(FRAME_BUFFER_BOOT_INFO, &size));
	if (info == NULL || size != sizeof(*info))
		return B_NOT_SUPPORTED;
	if (!BuildBootFrameBufferInfo(info->physical_frame_buffer, info->width,
			info->height, info->depth, info->bytes_per_row, sBootInfo))
		return B_BAD_VALUE;
	return B_OK;
}


void
uninit_driver()
{
}


const char**
publish_devices()
{
	return sDevices;
}


device_hooks*
find_device(const char* name)
{
	return strcmp(name, BOOT_GRAPHICS_DEVICE) == 0 ? &sHooks : NULL;
}
