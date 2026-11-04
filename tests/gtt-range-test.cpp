// SPDX-License-Identifier: MIT
#include "intel_gtt.h"

#include <stdio.h>


int
main()
{
	const uint64_t mib = 1ULL << 20;
	const uint64_t base = 0x400000000ULL;
	const uint64_t sentinel = 0x12345678;
	struct Test {
		const char* name;
		uint64_t base;
		uint64_t barSize;
		uint64_t tableSize;
		bool valid;
		uint64_t expected;
	};
	const Test tests[] = {
		{"2 MiB table above 4 GiB", base, 16 * mib, 2 * mib, true,
			base + 8 * mib},
		{"4 MiB table", base, 16 * mib, 4 * mib, true, base + 8 * mib},
		{"8 MiB table fills BAR", base, 16 * mib, 8 * mib, true,
			base + 8 * mib},
		{"last representable byte", UINT64_MAX - 16 * mib + 1,
			16 * mib, 8 * mib, true, UINT64_MAX - 8 * mib + 1},
		{"empty table", base, 16 * mib, 0, false, sentinel},
		{"oversized table", base, 32 * mib, 8 * mib + 1, false, sentinel},
		{"empty BAR", base, 0, 2 * mib, false, sentinel},
		{"legacy BAR", base, 4 * mib, 2 * mib, false, sentinel},
		{"BAR ends at table start", base, 8 * mib, 2 * mib, false,
			sentinel},
		{"BAR short by one byte", base, 16 * mib - 1, 8 * mib, false,
			sentinel},
		{"table end overflow", UINT64_MAX - 16 * mib + 2,
			16 * mib, 8 * mib, false, sentinel},
		{"table start overflow", UINT64_MAX - 8 * mib + 1,
			16 * mib, 2 * mib, false, sentinel},
		{"huge table", base, UINT64_MAX, UINT64_MAX, false, sentinel},
	};
	unsigned failures = 0;
	unsigned count = sizeof(tests) / sizeof(tests[0]);
	for (unsigned i = 0; i < count; i++) {
		uint64_t output = sentinel;
		bool valid = intel_gen8_gtt_base(tests[i].base, tests[i].barSize,
			tests[i].tableSize, &output);
		bool pass = valid == tests[i].valid && output == tests[i].expected;
		printf("%s %s\n", pass ? "PASS" : "FAIL", tests[i].name);
		if (!pass)
			failures++;
	}
	bool nullRejected = !intel_gen8_gtt_base(base, 16 * mib, 2 * mib, NULL);
	printf("%s null output\n", nullRejected ? "PASS" : "FAIL");
	if (!nullRejected)
		failures++;
	printf("%u GGTT range checks, %u failures\n", count + 1, failures);
	return failures == 0 ? 0 : 1;
}
