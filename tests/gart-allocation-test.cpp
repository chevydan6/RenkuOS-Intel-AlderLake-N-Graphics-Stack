// SPDX-License-Identifier: MIT
// Real allocation/free method bodies with injected VM and allocator failures.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using addr_t = uintptr_t;
using phys_addr_t = uint64_t;
using status_t = int32_t;
using int32 = int32_t;
using uint32 = uint32_t;
static constexpr size_t B_PAGE_SIZE = 4096;
static constexpr status_t B_OK = 0;
static constexpr status_t B_NO_MEMORY = -1;
static constexpr uint32 B_APERTURE_NEED_PHYSICAL = 2;
static constexpr uint32 PAGE_STATE_WIRED = 1;
static constexpr uint32 VM_PAGE_ALLOC_CLEAR = 2;
static constexpr uint32 VM_PRIORITY_SYSTEM = 1;
#define B_HAIKU_PHYSICAL_BITS 64
#define TRACE(...) ((void)0)
#define ERROR(...) ((void)0)
#define DEBUG_PAGE_ACCESS_TRANSFER(...) ((void)0)

struct vm_page { bool live; };
struct physical_address_restrictions { phys_addr_t high_address; };
struct aperture_memory {
	addr_t base;
	size_t size;
	uint32 flags;
	union { vm_page* page; vm_page** pages; };
};
struct aperture_info { addr_t base; size_t reserved_size; };
class Aperture {
public:
	bool _AdaptToReserved(addr_t&, size_t&, int32* = NULL);
	status_t AllocateMemory(aperture_memory*, uint32);
	void _Free(aperture_memory*);
	aperture_info fInfo;
};

static bool sFailTable, sFailPages;
static unsigned sChecks, sFailures, sAllocations, sPageCalls, sFreeCalls;
static vm_page sPages[4];
static void* sTable;

static void
Check(bool condition, const char* description)
{
	sChecks++;
	if (!condition) {
		printf("FAIL %s\n", description);
		sFailures++;
	}
}

static void*
AllocateTable(size_t size)
{
	if (sFailTable)
		return NULL;
	sTable = malloc(size);
	if (sTable != NULL)
		sAllocations++;
	return sTable;
}

static void
FreeTable(void* pointer)
{
	Check(pointer == sTable && sAllocations == 1, "free owns table");
	free(pointer);
	sTable = NULL;
	sAllocations--;
}

static vm_page*
vm_page_allocate_page_run(uint32 flags, uint32 count,
	physical_address_restrictions* restrictions, uint32 priority)
{
	sPageCalls++;
	Check(count == 4 && flags == (PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR)
		&& priority == VM_PRIORITY_SYSTEM
		&& restrictions->high_address == (uint64_t(1) << 32),
		"page allocation contract preserved");
	if (sFailPages)
		return NULL;
	for (vm_page& page : sPages)
		page.live = true;
	return sPages;
}

static void
vm_page_free(void*, vm_page* page)
{
	Check(page >= sPages && page < sPages + 4 && page->live,
		"free owns live page");
	page->live = false;
	sFreeCalls++;
}

#define malloc AllocateTable
#define free FreeTable
#ifndef GART_ALLOCATION_METHODS
#define GART_ALLOCATION_METHODS "generated/gart-allocation-current.inc"
#endif
#include GART_ALLOCATION_METHODS
#undef malloc
#undef free

int
main()
{
	Aperture aperture = {{0x100000, 0}};
	for (unsigned contiguous = 0; contiguous < 2; contiguous++) {
		for (unsigned fault = 0; fault < 3; fault++) {
			sFailTable = fault == 1;
			sFailPages = fault == 2;
			sPageCalls = sFreeCalls = 0;
			uint32 flags = contiguous ? B_APERTURE_NEED_PHYSICAL : 0;
			aperture_memory memory = {0x100000, 4 * B_PAGE_SIZE, flags, {NULL}};
			status_t result = aperture.AllocateMemory(&memory, flags);
			bool fail = sFailPages || (sFailTable && !contiguous);
			Check(result == (fail ? B_NO_MEMORY : B_OK), "allocation result");
			Check(sPageCalls == (sFailTable && !contiguous ? 0U : 1U),
				"table failure stops VM allocation");
			Check(((memory.flags & ALLOCATED_APERTURE) != 0) == !fail,
				"allocation ownership flag");
			if (fail) {
				Check(memory.pages == NULL, "failure clears page pointer");
				Check(sAllocations == 0, "failure releases pointer table");
			}
			aperture._Free(&memory);
			Check(sFreeCalls == (fail ? 0U : 4U), "free only allocated pages");
			Check((memory.flags & ALLOCATED_APERTURE) == 0, "free clears ownership");
			Check(sAllocations == 0, "cleanup releases pointer table");
			unsigned freed = sFreeCalls;
			aperture._Free(&memory);
			Check(sFreeCalls == freed, "repeat free does not release pages twice");
			// Dispose the original-code control's intentionally detected leak,
			// preserving sanitizer usefulness for other unexpected failures.
			if (sTable != NULL) {
				free(sTable);
				sTable = NULL;
				sAllocations = 0;
			}
		}
	}
	printf("%u GART allocation checks, %u failures; mock VM allocation\n",
		sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}
