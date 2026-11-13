/* SPDX-License-Identifier: MIT */
#ifndef RENKU_ASM_BITOPS_H
#define RENKU_ASM_BITOPS_H
/* Generic Linux non-atomic algorithms; native/compiler atomics for shared words.
 * The original Linux header checks the non-atomic function prototypes. */
#include <asm-generic/bitops/__ffs.h>
#include <asm-generic/bitops/__fls.h>
#include <asm-generic/bitops/ffz.h>
#include <asm-generic/bitops/fls.h>
#include <asm-generic/bitops/fls64.h>
#include <asm-generic/bitops/hweight.h>
#include <asm-generic/bitops/non-atomic.h>

#define RENKU_ATOMIC_BIT_OP(name, builtin) \
static inline void name(unsigned long nr, volatile unsigned long *addr) \
{ __atomic_fetch_##builtin(addr + BIT_WORD(nr), BIT_MASK(nr), __ATOMIC_RELAXED); }
RENKU_ATOMIC_BIT_OP(set_bit, or)
RENKU_ATOMIC_BIT_OP(change_bit, xor)
#undef RENKU_ATOMIC_BIT_OP
static inline void clear_bit(unsigned long nr, volatile unsigned long *addr)
{ __atomic_fetch_and(addr + BIT_WORD(nr), ~BIT_MASK(nr), __ATOMIC_RELAXED); }
#define RENKU_TEST_BIT_OP(name, builtin, mask) \
static inline bool name(unsigned long nr, volatile unsigned long *addr) \
{ return !!(__atomic_fetch_##builtin(addr + BIT_WORD(nr), mask, __ATOMIC_SEQ_CST) & BIT_MASK(nr)); }
RENKU_TEST_BIT_OP(test_and_set_bit, or, BIT_MASK(nr))
RENKU_TEST_BIT_OP(test_and_clear_bit, and, ~BIT_MASK(nr))
RENKU_TEST_BIT_OP(test_and_change_bit, xor, BIT_MASK(nr))
#undef RENKU_TEST_BIT_OP
static inline void clear_bit_unlock(unsigned long nr, volatile unsigned long *addr)
{ __atomic_fetch_and(addr + BIT_WORD(nr), ~BIT_MASK(nr), __ATOMIC_RELEASE); }
/* All other flags in this word must be protected by the same bit lock. */
static inline void __clear_bit_unlock(unsigned long nr, volatile unsigned long *addr)
{
	volatile unsigned long *word = addr + BIT_WORD(nr);
	unsigned long old = __atomic_load_n(word, __ATOMIC_RELAXED);
	__atomic_store_n(word, old & ~BIT_MASK(nr), __ATOMIC_RELEASE);
}
static inline bool test_and_set_bit_lock(unsigned long nr, volatile unsigned long *addr)
{ return !!(__atomic_fetch_or(addr + BIT_WORD(nr), BIT_MASK(nr), __ATOMIC_ACQUIRE) & BIT_MASK(nr)); }
#endif
