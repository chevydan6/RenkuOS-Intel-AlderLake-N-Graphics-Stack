/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_BITOPS_H
#define RENKU_LINUX_BITOPS_H
/* Only the reached test_bit/bit mutation interfaces in kpi_base are supplied. */
#include <kpi_base.h>
#include <asm/barrier.h>
static inline bool test_bit_acquire(unsigned int bit, const unsigned long *p)
{ return !!(__atomic_load_n(p + bit / 64, __ATOMIC_ACQUIRE) & (1UL << (bit % 64))); }
static inline bool test_and_set_bit(unsigned int bit, unsigned long *p)
{ return !!(__atomic_fetch_or(p + bit / 64, 1UL << (bit % 64), __ATOMIC_SEQ_CST) & (1UL << (bit % 64))); }
#define GENMASK_ULL(high, low) ((~0ULL << (low)) & (~0ULL >> (63 - (high))))
static inline s64 sign_extend64(u64 value, int index)
{
	unsigned int shift = 63 - index;
	return (s64)(value << shift) >> shift;
}
#endif
