/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_BITOPS_H
#define RENKU_LINUX_BITOPS_H
/* Only the reached test_bit/bit mutation interfaces in kpi_base are supplied. */
#include <kpi_base.h>
#define GENMASK_ULL(high, low) ((~0ULL << (low)) & (~0ULL >> (63 - (high))))
static inline s64 sign_extend64(u64 value, int index)
{
	unsigned int shift = 63 - index;
	return (s64)(value << shift) >> shift;
}
#endif
