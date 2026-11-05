// SPDX-License-Identifier: MIT
// Native userspace inventory. No PCI writes, raw physical mappings or GPU work.

#include <OS.h>
#include <graphic_driver.h>
#include <poke.h>
#include <vesa_info.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "BootGraphicsDriver.h"


class File {
public:
	File(const char* path) : fFD(open(path, O_RDONLY)) {}
	~File() { if (fFD >= 0) close(fFD); }
	int Get() const { return fFD; }
private:
	File(const File&) = delete;
	File& operator=(const File&) = delete;
	int fFD;
};


class Area {
public:
	Area(area_id id) : fID(id) {}
	~Area() { if (fID >= 0) delete_area(fID); }
	area_id Get() const { return fID; }
private:
	Area(const Area&) = delete;
	Area& operator=(const Area&) = delete;
	area_id fID;
};


static bool
ReadPCI(int fd, uint8 device, uint8 offset, uint8 size, uint32& value)
{
	pci_io_args request = {};
	request.signature = POKE_SIGNATURE;
	request.device = device;
	request.offset = offset;
	request.size = size;
	if (ioctl(fd, POKE_PCI_READ_CONFIG, &request, sizeof(request)) < 0) {
		fprintf(stderr, "PCI read failed: %s\n", strerror(errno));
		return false;
	}
	value = request.value;
	return true;
}


static int
InventoryPCI(int fd)
{
	uint32 bridge, gpu;
	if (!ReadPCI(fd, 0, 0, 4, bridge) || !ReadPCI(fd, 2, 0, 4, gpu))
		return 1;
	printf("bridge_id=%08" PRIx32 "\ngpu_id=%08" PRIx32 "\n", bridge, gpu);
	if (bridge != 0x461c8086 || gpu != 0x46d18086) {
		puts("target_present=false");
		return 3;
	}
	puts("target_present=true");
	struct Register {
		const char* name;
		uint8 device;
		uint8 offset;
		uint8 size;
	};
	const Register registers[] = {
		{"bridge_ggc", 0, 0x50, 2}, {"gpu_ggc", 2, 0x50, 2},
		{"bridge_bdsm", 0, 0xb0, 4}, {"bridge_bgsm", 0, 0xb4, 4},
		{"gpu_bdsm_low", 2, 0xc0, 4}, {"gpu_bdsm_high", 2, 0xc4, 4},
		{"gpu_command", 2, 0x04, 2}, {"gpu_revision_class", 2, 0x08, 4},
		{"bar0_low", 2, 0x10, 4}, {"bar0_high", 2, 0x14, 4},
		{"bar2_low", 2, 0x18, 4}, {"bar2_high", 2, 0x1c, 4}
	};
	for (const Register& reg : registers) {
		uint32 value;
		if (!ReadPCI(fd, reg.device, reg.offset, reg.size, value))
			return 1;
		printf("%s=%08" PRIx32 "\n", reg.name, value);
	}
	return 0;
}


static int
InventoryBootInfo()
{
	File device(BOOT_GRAPHICS_DEVICE_PATH);
	if (device.Get() < 0) {
		fprintf(stderr, "boot diagnostic open failed: %s\n", strerror(errno));
		return 1;
	}
	BootFrameBufferInfo info = {};
	if (ioctl(device.Get(), BOOT_GRAPHICS_GET_BOOT_INFO, &info, sizeof(info)) < 0)
		return 1;
	if (info.version != 1 || info.size != sizeof(info) || info.reserved != 0
		|| info.flags != kBootFrameBufferSnapshot)
		return 1;
	printf("boot_physical_base=%016" PRIx64 "\nboot_visible_bytes=%" PRIu64
		"\nboot_width=%" PRIu32 "\nboot_height=%" PRIu32 "\nboot_depth=%" PRIu32
		"\nboot_bytes_per_row=%" PRIu32 "\n", info.physicalBase,
		info.visibleBytes, info.width, info.height, info.depth, info.bytesPerRow);
	puts("current_scanout_verified=false\nggtt_translation_verified=false");
	return 0;
}


static int
InventoryFrameBuffer(const char* path)
{
	// Only use the already active firmware graphics device on a running desktop.
	File graphics(path);
	if (graphics.Get() < 0) {
		fprintf(stderr, "graphics open failed: %s\n", strerror(errno));
		return 1;
	}
	char signature[B_FILE_NAME_LENGTH] = {};
	if (ioctl(graphics.Get(), B_GET_ACCELERANT_SIGNATURE, signature,
			sizeof(signature)) < 0)
		return 1;
	const char* expected = strcmp(path, "/dev/graphics/vesa") == 0
		? "vesa.accelerant" : "framebuffer.accelerant";
	if (strncmp(signature, expected, sizeof(signature)) != 0) {
		fputs("unexpected accelerant signature\n", stderr);
		return 1;
	}
	area_id shared;
	if (ioctl(graphics.Get(), VESA_GET_PRIVATE_DATA, &shared, sizeof(shared)) < 0)
		return 1;
	void* data = NULL;
	Area sharedClone(clone_area("gpu inventory metadata", &data, B_ANY_ADDRESS,
		B_READ_AREA, shared));
	area_info sharedInfo;
	if (sharedClone.Get() < 0
		|| get_area_info(sharedClone.Get(), &sharedInfo) != B_OK
		|| sharedInfo.size < sizeof(vesa_shared_info))
		return 1;
	const vesa_shared_info* info = static_cast<const vesa_shared_info*>(data);
	// Observational snapshot only; mode changes can race these metadata reads.
	uint32 width = info->current_mode.virtual_width;
	uint32 height = info->current_mode.virtual_height;
	uint32 stride = info->bytes_per_row;

	printf("device=%s\naccelerant=%s\nwidth=%" PRIu32 "\nheight=%" PRIu32
		"\nbytes_per_row=%" PRIu32 "\n", path, signature, width, height, stride);
	puts("physical_address_available=false\nggtt_translation_verified=false");
	return 0;
}


int
main(int argc, char** argv)
{
	bool pci = argc == 2 && strcmp(argv[1], "--pci") == 0;
	bool boot = argc == 2 && strcmp(argv[1], "--boot-info") == 0;
	bool frame = argc == 3 && strcmp(argv[1], "--framebuffer") == 0
		&& (strcmp(argv[2], "/dev/graphics/vesa") == 0
			|| strcmp(argv[2], "/dev/graphics/framebuffer") == 0);
	if (!pci && !frame && !boot) {
		fputs("usage: boot-graphics-inventory --pci\n"
			"       boot-graphics-inventory --boot-info\n"
			"       boot-graphics-inventory --framebuffer "
			"/dev/graphics/{vesa,framebuffer}\n", stderr);
		return 2;
	}
	if (geteuid() != 0) {
		fputs("root required for existing poke interface\n", stderr);
		return 1;
	}
	puts("schema=renku-boot-graphics-v1\ngpu_commands_submitted=false");
	if (boot)
		return InventoryBootInfo();
	if (frame)
		return InventoryFrameBuffer(argv[2]);
	// poke temporarily raises this thread's IOPL; close restores it on x86.
	File poke(POKE_DEVICE_FULLNAME);
	if (poke.Get() < 0) {
		fprintf(stderr, "poke open failed: %s\n", strerror(errno));
		return 1;
	}
	return InventoryPCI(poke.Get());
}
