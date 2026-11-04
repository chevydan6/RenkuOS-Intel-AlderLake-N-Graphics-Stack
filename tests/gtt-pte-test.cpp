// SPDX-License-Identifier: MIT
#include "intel_gtt.h"

#include <stdio.h>
#include <string.h>


static unsigned sChecks;
static unsigned sFailures;

static void
check(bool pass, const char* name)
{
	sChecks++;
	if (!pass)
		sFailures++;
	printf("%s %s\n", pass ? "PASS" : "FAIL", name);
}


int
main()
{
	const uint64_t page = 4096;
	const uint64_t limit = 1ULL << 39;
	uint64_t pte = 0;
	check(intel_gen8_gtt_pte(0, &pte) && pte == 1, "physical page zero");
	check(intel_gen8_gtt_pte(0x100001000ULL, &pte)
		&& pte == 0x100001001ULL, "address above 4 GiB remains intact");
	check(intel_gen8_gtt_pte(limit - page, &pte)
		&& pte == limit - page + 1, "last 39-bit page");
	uint64_t saved = pte;
	check(!intel_gen8_gtt_pte(limit, &pte) && pte == saved,
		"reject DMA overflow without changing output");
	check(!intel_gen8_gtt_pte(1, &pte) && pte == saved,
		"reject unaligned physical address");
	check(!intel_gen8_gtt_pte(0, NULL), "reject null PTE output");
	check(intel_gen8_gtt_layout(4, 4 * page, page), "valid aperture layout");
	check(intel_gen8_gtt_layout(1ULL << 20, 1ULL << 32, 0),
		"maximum 4 GiB aperture");
	check(!intel_gen8_gtt_layout(0, page, 0), "empty table");
	check(!intel_gen8_gtt_layout((1ULL << 20) + 1, page, 0), "oversized table");
	check(!intel_gen8_gtt_layout(4, 0, 0), "empty aperture");
	check(!intel_gen8_gtt_layout(4, 4 * page - 1, 0), "unaligned aperture");
	check(!intel_gen8_gtt_layout(3, 4 * page, 0), "table cannot cover aperture");
	check(!intel_gen8_gtt_layout(4, 4 * page, 1), "unaligned reservation");
	check(!intel_gen8_gtt_layout(4, 4 * page, 5 * page), "reservation beyond aperture");

	uint64_t slots[6];
	for (unsigned i = 0; i < 6; i++)
		slots[i] = 0xabcdef0123456789ULL;
	uint64_t original[6];
	memcpy(original, slots, sizeof(slots));
	struct Rejected {
		uint64_t offset;
		uint64_t physical;
		const char* name;
	};
	const Rejected rejected[] = {
		{0, page, "reserved mapping remains unchanged"},
		{page + 1, page, "unaligned offset writes nothing"},
		{4 * page, page, "aperture end writes nothing"},
		{UINT64_MAX, page, "huge offset writes nothing"},
		{page, page + 1, "unaligned physical page writes nothing"},
		{page, limit, "unaddressable physical page writes nothing"},
	};
	for (unsigned i = 0; i < sizeof(rejected) / sizeof(rejected[0]); i++) {
		check(!intel_gen8_gtt_write(slots + 1, 4, 4 * page, page,
				rejected[i].offset, rejected[i].physical)
			&& memcmp(slots, original, sizeof(slots)) == 0, rejected[i].name);
	}
	check(!intel_gen8_gtt_write(NULL, 4, 4 * page, page, page, page),
		"reject null table");
	check(!intel_gen8_gtt_write((uint64_t*)((char*)slots + 1), 4,
		4 * page, page, page, page), "reject unaligned table");
	check(!intel_gen8_gtt_write(slots + 1, 3, 4 * page, page, page, page)
		&& memcmp(slots, original, sizeof(slots)) == 0,
		"invalid layout writes nothing");
	check(intel_gen8_gtt_write(slots + 1, 4, 4 * page, page, page,
		0x4000001000ULL), "write 64-bit PTE");
	original[2] = 0x4000001001ULL;
	check(memcmp(slots, original, sizeof(slots)) == 0,
		"8-byte stride, upper address bits and adjacent guards");
	check(intel_gen8_gtt_write(slots + 1, 4, 4 * page, page, 3 * page,
		limit - page), "write last aperture PTE");
	original[4] = limit - page + 1;
	check(memcmp(slots, original, sizeof(slots)) == 0,
		"last entry preserves trailing guard");
	printf("%u GGTT PTE checks, %u failures\n", sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}
