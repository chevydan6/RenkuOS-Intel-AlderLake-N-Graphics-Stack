// SPDX-License-Identifier: MIT
// Production bind/unbind/delete methods; simulated pages and faulting callbacks.
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

using addr_t = uintptr_t;
using phys_addr_t = uint64_t;
using status_t = int32_t;
using int32 = int32_t;
using uint32 = uint32_t;
using aperture_id = int32_t;
using area_id = int32_t;
static constexpr size_t B_PAGE_SIZE = 4096;
static constexpr unsigned PAGE_SHIFT = 12;
static constexpr status_t B_OK = 0;
static constexpr status_t B_BAD_VALUE = -1;
static constexpr status_t B_ERROR = -2;
static constexpr status_t B_ENTRY_NOT_FOUND = -3;
static constexpr status_t B_NO_MEMORY = -4;
static constexpr uint32 B_APERTURE_NON_RESERVED = 0x01;
static constexpr uint32 B_APERTURE_NEED_PHYSICAL = 0x02;
static constexpr phys_addr_t kPhysicalBase = 0x100000;
#define TRACE(...) ((void)0)
#define ERROR(...) ((void)0)
#define ROUNDUP(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

struct vm_page { uint64_t physical_page_number; };
struct physical_entry { phys_addr_t address; size_t size; };

static status_t
get_memory_map(const void* address, size_t size, physical_entry* entry, int)
{
	entry->address = reinterpret_cast<addr_t>(address);
	entry->size = size;
	return B_OK;
}

struct aperture_memory {
	aperture_memory* next;
	addr_t base;
	size_t size;
	uint32 flags;
	size_t bound_size;
	union { vm_page* page; vm_page** pages; };
};

struct aperture_info { addr_t base; size_t reserved_size; };
struct bus_module {
	status_t (*bind_page)(void*, addr_t, phys_addr_t);
	status_t (*unbind_page)(void*, addr_t);
	void (*flush_tlbs)(void*);
};
struct HashTable {
	void* cookie;
	void Remove(aperture_memory*);
};
struct TestLock {};
class Autolock {
public:
	explicit Autolock(TestLock&) {}
};

#ifndef GART_DELETE_RESULT
#define GART_DELETE_RESULT status_t
#endif
#ifndef GART_HAS_DELETE_ALL
#define GART_HAS_DELETE_ALL 1
#endif
class Aperture {
public:
	~Aperture();
	status_t BindMemory(aperture_memory*, addr_t, size_t);
	status_t UnbindMemory(aperture_memory*);
	GART_DELETE_RESULT DeleteMemory(aperture_memory*);
	status_t DeleteAllMemory();
	aperture_memory* CreateMemory(size_t, size_t, uint32);
	aperture_memory* GetMemory(addr_t base)
	{
		for (aperture_memory* item = fFirstMemory; item != NULL; item = item->next) {
			if (item->base == base)
				return item;
		}
		return NULL;
	}
	bool _AdaptToReserved(addr_t&, size_t&, int32* = NULL);
	void _Free(aperture_memory*);
	void _Remove(aperture_memory*);
	addr_t Base() const { return fInfo.base; }
	size_t Size() const { return 16 * B_PAGE_SIZE; }
	TestLock& Lock() { return fLock; }
	aperture_info fInfo;
	bus_module* fModule;
	void* fPrivateAperture;
	HashTable fHashTable;
	aperture_memory* fFirstMemory;
	TestLock fLock = {};
};

struct ApertureTable {
	Aperture* value;
	Aperture* Lookup(aperture_id id) { return id == 1 ? value : NULL; }
	void Remove(Aperture* aperture) { if (value == aperture) value = NULL; }
};
TestLock sLock;
ApertureTable sApertureHashTable = {};
static Aperture* get_aperture(aperture_id id) { return sApertureHashTable.Lookup(id); }
static status_t get_area_base_and_size(area_id, addr_t&, size_t&) { return B_BAD_VALUE; }

#ifndef GART_LIFECYCLE_METHODS
#define GART_LIFECYCLE_METHODS "generated/gart-lifecycle-current.inc"
#endif
#include GART_LIFECYCLE_METHODS

struct Backend {
	phys_addr_t mapped[16];
	unsigned bindCalls;
	unsigned unbindCalls;
	unsigned flushes;
	unsigned frees;
	unsigned removals;
	unsigned destructions;
	bool freedWhileMapped;
	bool destroyedWhileMapped;
	int failBindAt;
	int failUnbindPage;
};

static bool
owned_mappings(const Backend& state)
{
	for (phys_addr_t entry : state.mapped) {
		if (entry >= kPhysicalBase && entry < kPhysicalBase + 4 * B_PAGE_SIZE)
			return true;
	}
	return false;
}

Aperture::~Aperture()
{
	Backend& state = *static_cast<Backend*>(fPrivateAperture);
	state.destructions++;
	state.destroyedWhileMapped |= owned_mappings(state);
}

void
Aperture::_Free(aperture_memory* memory)
{
	if ((memory->flags & ALLOCATED_APERTURE) == 0)
		return;
	Backend& state = *static_cast<Backend*>(fPrivateAperture);
	state.frees++;
	state.freedWhileMapped |= owned_mappings(state);
}

aperture_memory*
Aperture::CreateMemory(size_t size, size_t, uint32 flags)
{
	// Fixture metadata allocator: all tests start with this slot available.
	aperture_memory* memory = new aperture_memory{};
	memory->base = Base() + 3 * B_PAGE_SIZE;
	memory->size = size;
	memory->flags = flags;
	memory->next = fFirstMemory;
	fFirstMemory = memory;
	return memory;
}

void
HashTable::Remove(aperture_memory*)
{
	static_cast<Backend*>(cookie)->removals++;
}

static status_t
bind(void* cookie, addr_t offset, phys_addr_t physical)
{
	Backend& state = *static_cast<Backend*>(cookie);
	unsigned call = state.bindCalls++;
	if (static_cast<int>(call) == state.failBindAt)
		return B_ERROR;
	if (offset % B_PAGE_SIZE != 0 || offset / B_PAGE_SIZE >= 16)
		return B_BAD_VALUE;
	state.mapped[offset / B_PAGE_SIZE] = physical;
	return B_OK;
}

static status_t
unbind(void* cookie, addr_t offset)
{
	Backend& state = *static_cast<Backend*>(cookie);
	state.unbindCalls++;
	if (offset % B_PAGE_SIZE != 0 || offset / B_PAGE_SIZE >= 16)
		return B_BAD_VALUE;
	unsigned page = offset / B_PAGE_SIZE;
	if (static_cast<int>(page) == state.failUnbindPage)
		return B_ERROR;
	state.mapped[page] = 0;
	return B_OK;
}

static void
flush(void* cookie)
{
	static_cast<Backend*>(cookie)->flushes++;
}

static unsigned sChecks;
static unsigned sFailures;

static void
check(bool pass, const char* label)
{
	sChecks++;
	if (!pass) {
		sFailures++;
		fprintf(stderr, "FAIL %s\n", label);
	}
}

int
main()
{
	bus_module module = {bind, unbind, flush};
	vm_page pages[4];
	vm_page* pointers[4];
	for (unsigned i = 0; i < 4; i++) {
		pages[i].physical_page_number = (kPhysicalBase >> PAGE_SHIFT) + i;
		pointers[i] = &pages[i];
	}
	const bool options[] = {false, true};
	{
		Backend state = {};
		state.failBindAt = 2;
		state.failUnbindPage = -1;
		Aperture aperture = {{0x400000, 0}, &module, &state, {&state}, NULL};
		sApertureHashTable.value = &aperture;
		addr_t output = 0xdead;
		check(bind_aperture(1, -1, kPhysicalBase, 4 * B_PAGE_SIZE, 0, 0, &output)
			== B_ERROR, "public borrowed bind propagates backend failure");
		check(output == 0xdead, "failed public bind preserves output handle");
		check(!owned_mappings(state), "failed borrowed bind clears installed prefix");
		check(aperture.fFirstMemory == NULL && state.removals == 1,
			"failed borrowed bind removes new metadata after cleanup");
		check(state.frees == 0, "borrowed cleanup does not free caller backing");
		// Original control leaks metadata; explicitly dispose it for the test.
		if (aperture.fFirstMemory != NULL)
			aperture.DeleteMemory(aperture.fFirstMemory);
		sApertureHashTable.value = NULL;
	}
	{
		Backend state = {};
		state.failBindAt = -1;
		state.failUnbindPage = -1;
		Aperture aperture = {{0x400000, 0}, &module, &state, {&state}, NULL};
		sApertureHashTable.value = &aperture;
		aperture_memory* memory = aperture.CreateMemory(B_PAGE_SIZE, 0, ALLOCATED_APERTURE);
		memory->pages = pointers;
		check(aperture.BindMemory(memory, 0, 0) == B_OK, "public rebind fixture binds");
		addr_t output = 0xdead;
		check(bind_aperture(1, -1, kPhysicalBase, B_PAGE_SIZE, 0, memory->base, &output)
			== B_BAD_VALUE, "public reserved rebind rejected");
		check(output == 0xdead, "rejected public rebind preserves output handle");
		check(state.bindCalls == 1 && state.unbindCalls == 0 && owned_mappings(state),
			"rejected public rebind preserves existing mapping");
		aperture.DeleteMemory(memory);
		sApertureHashTable.value = NULL;
	}
	for (bool contiguous : options) {
		for (int failAfter = 0; failAfter <= 4; failAfter++) {
			for (bool cleanupFailure : options) {
				Backend state = {};
				state.failBindAt = failAfter == 4 ? -1 : failAfter;
				// Fail the second reverse-unbind after one successful clear.
				state.failUnbindPage = cleanupFailure && failAfter >= 2
					? 3 + failAfter - 2 : -1;
				Aperture aperture = {{0x400000, 3 * B_PAGE_SIZE}, &module, &state, {&state}, NULL};
				aperture_memory* memory = new aperture_memory{};
				aperture.fFirstMemory = memory;
				memory->base = aperture.Base() + 3 * B_PAGE_SIZE;
				memory->size = 4 * B_PAGE_SIZE;
				memory->flags = ALLOCATED_APERTURE;
				if (contiguous) {
					memory->flags |= B_APERTURE_NEED_PHYSICAL;
					memory->page = pages;
				} else
					memory->pages = pointers;
				status_t result = aperture.BindMemory(memory, 0, 0);
				check(result == (failAfter == 4 ? B_OK : B_ERROR), "bind result propagated");
				aperture.DeleteMemory(memory);
				bool retained = state.failUnbindPage >= 0;
				check(state.frees == (retained ? 0U : 1U), "cleanup preserves owned backing on failure");
				check(!state.freedWhileMapped, "no free with owned PTEs installed");
				check(state.removals == (retained ? 0U : 1U), "cleanup preserves allocation metadata");
				if (!retained)
					check(!owned_mappings(state), "completed cleanup removes every owned PTE");
				if (retained && state.removals == 0) {
					check(memory->bound_size == static_cast<size_t>(failAfter - 1) * B_PAGE_SIZE,
						"failed cleanup retains exact mapped prefix");
					check(state.flushes >= 1, "partial cleanup flushes successful clears");
					state.failUnbindPage = -1;
					aperture.DeleteMemory(memory);
					check(state.frees == 1 && state.removals == 1,
						"retry releases retained allocation once");
					check(!owned_mappings(state) && !state.freedWhileMapped,
						"retry clears mappings before freeing");
				}
			}
		}
	}
#if GART_HAS_DELETE_ALL
	{
		Backend state = {};
		state.failBindAt = -1;
		state.failUnbindPage = 3;
		Aperture* aperture = new Aperture{{0x400000, 0}, &module, &state, {&state}, NULL};
		sApertureHashTable.value = aperture;
		aperture_memory* memory = new aperture_memory{};
		aperture->fFirstMemory = memory;
		memory->base = aperture->Base() + 3 * B_PAGE_SIZE;
		memory->size = 4 * B_PAGE_SIZE;
		memory->flags = ALLOCATED_APERTURE;
		memory->pages = pointers;
		check(aperture->BindMemory(memory, 0, 0) == B_OK, "aperture unmap fixture binds");
		check(unmap_aperture(1) == B_ERROR, "aperture unmap returns cleanup failure");
		check(sApertureHashTable.value == aperture && state.destructions == 0,
			"failed unmap preserves aperture and registry entry");
		check(state.frees == 0 && owned_mappings(state), "failed unmap preserves backing");
		state.failUnbindPage = -1;
		check(unmap_aperture(1) == B_OK, "aperture unmap retry succeeds");
		check(sApertureHashTable.value == NULL && state.destructions == 1,
			"successful unmap removes and destroys aperture once");
		check(!state.destroyedWhileMapped && !state.freedWhileMapped && state.frees == 1,
			"unmap clears owned PTEs before resource destruction");
		check(unmap_aperture(1) == B_ENTRY_NOT_FOUND, "repeated unmap rejects missing aperture");
	}
	{
		Backend state = {};
		state.failBindAt = -1;
		state.failUnbindPage = -1;
		Aperture aperture = {{0x400000, 0}, &module, &state, {&state}, NULL};
		aperture_memory* memory = new aperture_memory{};
		aperture.fFirstMemory = memory;
		memory->base = aperture.Base() + 3 * B_PAGE_SIZE;
		memory->size = B_PAGE_SIZE;
		check(aperture.BindMemory(memory, kPhysicalBase, 2 * B_PAGE_SIZE) == B_BAD_VALUE,
			"oversized bind rejected");
		check(aperture.BindMemory(memory, kPhysicalBase, B_PAGE_SIZE - 1) == B_BAD_VALUE,
			"unaligned bind length rejected");
		check(state.bindCalls == 0, "invalid binds leave mappings untouched");
		memory->flags = ALLOCATED_APERTURE;
		memory->pages = pointers;
		check(aperture.BindMemory(memory, 0, 0) == B_OK, "rebind fixture binds");
		check(aperture.BindMemory(memory, 0, 0) == B_BAD_VALUE,
			"already-bound allocation rejected");
		check(state.bindCalls == 1 && state.mapped[3] == kPhysicalBase,
			"rejected rebind preserves existing PTE");
		memory->bound_size = 2 * B_PAGE_SIZE;
		check(aperture.UnbindMemory(memory) == B_BAD_VALUE,
			"oversized tracked prefix rejected");
		memory->bound_size = B_PAGE_SIZE - 1;
		check(aperture.UnbindMemory(memory) == B_BAD_VALUE,
			"unaligned tracked prefix rejected");
		check(state.unbindCalls == 0 && owned_mappings(state),
			"invalid tracked prefix preserves mappings");
		memory->bound_size = B_PAGE_SIZE;
		memory->flags &= ~BIND_APERTURE;
		check(aperture.DeleteMemory(memory) == B_BAD_VALUE,
			"inconsistent binding flag rejects deletion");
		check(state.frees == 0 && state.removals == 0,
			"inconsistent binding flag retains owned backing");
		memory->flags |= BIND_APERTURE;
		aperture.DeleteMemory(memory);
		check(!owned_mappings(state) && !state.freedWhileMapped && state.frees == 1,
			"valid tracked prefix permits safe cleanup");
	}
	{
		Backend state = {};
		state.failBindAt = -1;
		state.failUnbindPage = 3;
		Aperture aperture = {{0x400000, 0}, &module, &state, {&state}, NULL};
		aperture_memory* first = new aperture_memory{};
		aperture_memory* second = new aperture_memory{};
		aperture.fFirstMemory = first;
		first->next = second;
		first->base = aperture.Base() + 3 * B_PAGE_SIZE;
		first->size = 2 * B_PAGE_SIZE;
		first->flags = ALLOCATED_APERTURE;
		first->pages = pointers;
		second->base = aperture.Base() + 7 * B_PAGE_SIZE;
		second->size = B_PAGE_SIZE;
		second->flags = ALLOCATED_APERTURE;
		second->pages = pointers;
		check(aperture.BindMemory(first, 0, 0) == B_OK, "teardown fixture binds");
		check(aperture.DeleteAllMemory() == B_ERROR, "aperture cleanup propagates failure");
		check(aperture.fFirstMemory == first && first->next == second,
			"failed aperture cleanup keeps allocation chain");
		check(state.frees == 0 && state.removals == 0,
			"failed aperture cleanup preserves backing and metadata");
		state.failUnbindPage = -1;
		check(aperture.DeleteAllMemory() == B_OK, "aperture cleanup retry succeeds");
		check(aperture.fFirstMemory == NULL && state.frees == 2 && state.removals == 2,
			"aperture cleanup releases each allocation once");
		check(!owned_mappings(state) && !state.freedWhileMapped,
			"aperture cleanup never frees mapped backing");
	}
#endif
	printf("%u GART lifecycle checks, %u failures; simulated backing and bus faults\n",
		sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}
