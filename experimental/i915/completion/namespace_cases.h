/* SPDX-License-Identifier: MIT */
#ifndef RENKU_NAMESPACE_CASES_H
#define RENKU_NAMESPACE_CASES_H
#include <linux/math.h>
#include <linux/align.h>
#include <asm/byteorder.h>
_Static_assert(ALIGN(17, 16) == 32 && round_up(17, 16) == 32, "constant alignment");
static void NamespaceCases(struct completion_report *r)
{
	_Static_assert(min(4, 7) == 4 && max(4, 7) == 7, "constant min/max");
	int a = 3, b = 7;
	CHECK(r, min(a++, b++) == 3 && a == 4 && b == 8);
	a = 3; b = 7;
	CHECK(r, max(a++, b++) == 7 && a == 4 && b == 8);
	CHECK(r, clamp(50, 2, 17) == 17 && clamp(-1, 2, 17) == 2);
	CHECK(r, roundup(17, 6) == 18 && rounddown(17, 6) == 12);
	CHECK(r, ALIGN_DOWN(31, 16) == 16 && IS_ALIGNED(32, 16));
	CHECK(r, lower_32_bits(0x123456789abcdef0ULL) == 0x9abcdef0u);
	CHECK(r, swab16(0x1234) == 0x3412 && swab32(0x12345678u) == 0x78563412u);
	CHECK(r, swab64(0x0123456789abcdefULL) == 0xefcdab8967452301ULL);
	CHECK(r, le64_to_cpu(cpu_to_le64(0x123456789abcdef0ULL)) == 0x123456789abcdef0ULL);
	CHECK(r, be32_to_cpu(cpu_to_be32(0x12345678u)) == 0x12345678u);
	BUILD_BUG_ON(sizeof(unsigned long) != 8);
}
#endif
