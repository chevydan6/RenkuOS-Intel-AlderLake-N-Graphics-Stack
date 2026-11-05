// SPDX-License-Identifier: MIT
// Exact initialization statements; mock device metadata and memory allocator.
#include <stdint.h>
#include <stdio.h>
#include <string.h>

using addr_t = uintptr_t;
using phys_addr_t = uint64_t;
using status_t = int32_t;
static constexpr size_t B_PAGE_SIZE = 4096;
static constexpr unsigned INTEL_GROUP_ALD = 1;
static constexpr unsigned INTEL_PRIMARY_RING_BUFFER = 0x2000;
static constexpr unsigned B_APERTURE_NEED_PHYSICAL = 2;
static constexpr status_t B_OK = 0;
static constexpr status_t B_NO_MEMORY = -1;
static constexpr status_t B_BAD_VALUE = -2;
#define TRACE(...) ((void)0)
#define ERROR(...) ((void)0)

struct DeviceType {
	unsigned group;
	bool InGroup(unsigned value) const { return group == value; }
};
struct ring_buffer {
	uint8_t* base;
	unsigned register_base;
	size_t size;
	addr_t offset;
};
struct overlay_registers { unsigned value; };
struct intel_shared_info {
	ring_buffer primary_ring_buffer;
	bool physicalOverlay;
	phys_addr_t physical_overlay_registers;
	addr_t overlay_offset;
	uint8_t* status_page;
	phys_addr_t physical_status_page;
	uint8_t* cursor_memory;
	phys_addr_t physical_cursor_memory;
};
struct intel_info {
	DeviceType device_type;
	intel_shared_info* shared_info;
	addr_t aperture_base;
	struct overlay_registers* overlay_registers;
};
struct Request { size_t size; unsigned flags; };
static Request sRequests[4];
static unsigned sCalls, sOverlayInit, sChecks, sFailures;
static int sFailAt;
alignas(4096) static uint8_t sBacking[4][16 * B_PAGE_SIZE];

static void
Check(bool condition, const char* message)
{
	sChecks++;
	if (!condition) {
		printf("FAIL %s\n", message);
		sFailures++;
	}
}

static status_t
intel_allocate_memory(intel_info&, size_t size, size_t, unsigned flags,
	addr_t* address, phys_addr_t* physical = NULL)
{
	unsigned call = sCalls++;
	if (call >= 4)
		return B_NO_MEMORY;
	sRequests[call] = {size, flags};
	if (address == NULL)
		return B_BAD_VALUE;
	if (int(call) == sFailAt)
		return B_NO_MEMORY;
	*address = reinterpret_cast<addr_t>(sBacking[call]);
	if (physical != NULL)
		*physical = 0x100000 + call * sizeof(sBacking[0]);
	return B_OK;
}

static bool
intel_uses_physical_overlay(const intel_shared_info& info)
{
	return info.physicalOverlay;
}

static void
init_overlay_registers(overlay_registers* registers)
{
	sOverlayInit++;
	registers->value = 0x12345678;
}

#ifndef KERNEL_RESOURCE_CODE
#define KERNEL_RESOURCE_CODE "generated/kernel-resources-current.inc"
#endif
#include KERNEL_RESOURCE_CODE

int
main()
{
	for (unsigned alderLake = 0; alderLake < 2; alderLake++) {
		for (unsigned physical = 0; physical < 2; physical++) {
			for (unsigned cursor = 0; cursor < 2; cursor++) {
				for (int fault = -1; fault < 4; fault++) {
					intel_shared_info shared = {};
					shared.physicalOverlay = physical;
					intel_info info = {{alderLake ? INTEL_GROUP_ALD : 2}, &shared,
						reinterpret_cast<addr_t>(sBacking), NULL};
					sCalls = sOverlayInit = 0;
					sFailAt = fault;
					memset(sBacking, 0xa5, sizeof(sBacking));
					InitializeRing(info);
					InitializeAuxiliary(info, cursor);
					if (alderLake) {
						Check(sCalls == cursor, "Alder Lake requests only cursor storage");
						Check(shared.primary_ring_buffer.base == NULL
							&& shared.primary_ring_buffer.size == 0
							&& shared.primary_ring_buffer.offset == 0,
							"Alder Lake ring remains empty");
						Check(info.overlay_registers == NULL && sOverlayInit == 0
							&& shared.overlay_offset == 0,
							"Alder Lake overlay remains absent");
						bool allocated = cursor && fault != 0;
						Check((shared.cursor_memory != NULL) == allocated,
							"Alder Lake cursor remains independent");
					} else {
						Check(sCalls == 3 + cursor, "legacy request count preserved");
						Check(sRequests[0].size == 16 * B_PAGE_SIZE
							&& sRequests[0].flags == 0, "legacy ring request preserved");
						Check((shared.primary_ring_buffer.base != NULL) == (fault != 0),
							"legacy ring failure behavior preserved");
						Check(sRequests[1].size == B_PAGE_SIZE
							&& sRequests[1].flags == (physical ? B_APERTURE_NEED_PHYSICAL : 0),
							"legacy overlay policy preserved");
						Check(sOverlayInit == (fault != 1 ? 1U : 0U),
							"legacy overlay initialization preserved");
						Check(sRequests[2].flags == B_APERTURE_NEED_PHYSICAL,
							"legacy status request preserved");
						Check((shared.cursor_memory != NULL) == (cursor && fault != 3),
							"legacy cursor preserved");
					}
					Check(shared.status_page == NULL && shared.physical_status_page == 0,
						"status page remains unallocated");
					if (cursor) {
						Check(sRequests[sCalls - 1].size == B_PAGE_SIZE
							&& sRequests[sCalls - 1].flags == B_APERTURE_NEED_PHYSICAL,
							"cursor request size and flags");
					}
				}
			}
		}
	}
	printf("%u kernel resource checks, %u failures; mock allocation only\n",
		sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}
