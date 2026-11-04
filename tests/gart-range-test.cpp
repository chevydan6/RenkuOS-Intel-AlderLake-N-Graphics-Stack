// SPDX-License-Identifier: MIT
// Actual production methods; mock only aperture metadata and bus callbacks.
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

using addr_t = uintptr_t;
using status_t = int32_t;
using int32 = int32_t;
using uint32 = uint32_t;
static constexpr size_t B_PAGE_SIZE = 4096;
static constexpr status_t B_OK = 0;
static constexpr status_t B_BAD_VALUE = -1;
static constexpr status_t B_ERROR = -2;
#define TRACE(...) ((void)0)

struct aperture_memory {
	addr_t base;
	size_t size;
	uint32 flags;
	size_t bound_size;
};

struct aperture_info {
	addr_t base;
	size_t reserved_size;
};

struct bus_module {
	status_t (*unbind_page)(void*, addr_t);
	void (*flush_tlbs)(void*);
};

class Aperture {
public:
	status_t UnbindMemory(aperture_memory* memory);
	bool _AdaptToReserved(addr_t& base, size_t& size, int32* offset = NULL);
	addr_t Base() const { return fInfo.base; }
	aperture_info fInfo;
	bus_module* fModule;
	void* fPrivateAperture;
};

#ifndef GART_METHODS
#define GART_METHODS "generated/gart-current.inc"
#endif
#include GART_METHODS

struct Backend {
	bool mapped[16];
	unsigned calls;
	unsigned flushes;
	int failAt;
};

static status_t
unbind(void* cookie, addr_t offset)
{
	Backend& backend = *static_cast<Backend*>(cookie);
	backend.calls++;
	if (offset % B_PAGE_SIZE != 0 || offset / B_PAGE_SIZE >= 16)
		return B_BAD_VALUE;
	unsigned page = offset / B_PAGE_SIZE;
	if (static_cast<int>(page) == backend.failAt)
		return B_ERROR;
	backend.mapped[page] = false;
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

static Backend
backend()
{
	Backend result = {};
	for (bool& entry : result.mapped)
		entry = true;
	result.failAt = -1;
	return result;
}

int
main()
{
	bus_module module = {unbind, flush};
	const unsigned cases[][3] = {
		{3, 2, 4}, {3, 0, 4}, {3, 1, 3}, // crosses reserved boundary
		{3, 3, 4}, {3, 7, 2}, {3, 0, 3}, {0, 0, 4}, {3, 14, 2}
	};
	for (const auto& item : cases) {
		Backend state = backend();
		Aperture aperture = {{0x100000, item[0] * B_PAGE_SIZE}, &module, &state};
		aperture_memory memory = {aperture.Base() + item[1] * B_PAGE_SIZE,
			item[2] * B_PAGE_SIZE, BIND_APERTURE, 0};
		unsigned start = item[1] < item[0] ? item[0] : item[1];
		unsigned end = item[1] + item[2];
		unsigned pages = end > start ? end - start : 0;
		memory.bound_size = pages * B_PAGE_SIZE;
		check(aperture.UnbindMemory(&memory) == B_OK, "unbind succeeds");
		check(state.calls == pages, "unbind adjusted page count");
		bool correct = true;
		for (unsigned i = 0; i < 16; i++)
			correct &= state.mapped[i] == !(i >= start && i < end);
		check(correct, "unbind preserves reserved and adjacent mappings");
		check((memory.flags & BIND_APERTURE) == 0, "unbind clears bound flag");
		check(state.flushes == (pages == 0 ? 0U : 1U), "unbind completion flush");
	}
	Backend state = backend();
	Aperture aperture = {{0x100000, 3 * B_PAGE_SIZE}, &module, &state};
	aperture_memory memory = {aperture.Base() + 3 * B_PAGE_SIZE, B_PAGE_SIZE, 0, 0};
	check(aperture.UnbindMemory(&memory) == B_BAD_VALUE, "unbound allocation rejected");
	check(state.calls == 0 && state.flushes == 0, "unbound allocation untouched");
	state.failAt = 3;
	memory.flags = BIND_APERTURE;
	memory.bound_size = B_PAGE_SIZE;
	check(aperture.UnbindMemory(&memory) == B_ERROR, "backend failure propagated");
	check((memory.flags & BIND_APERTURE) != 0, "failed unbind keeps bound flag");
	check(state.calls == 1 && state.mapped[3], "failed first page untouched");
	check(state.flushes == 0, "no flush when no page changed");
	printf("%u GART range checks, %u failures; production methods, mock callbacks\n",
		sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}
