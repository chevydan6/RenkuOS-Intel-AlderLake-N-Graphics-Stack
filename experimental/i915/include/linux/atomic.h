/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_ATOMIC_H
#define RENKU_LINUX_ATOMIC_H
#include <kpi_base.h>
typedef struct { int counter; } atomic_t;
#define ATOMIC_INIT(n) { .counter = (n) }
_Static_assert(sizeof(int) == 4, "Linux atomic_t requires 32-bit int");

static inline void renku_linux_atomic_set(atomic_t *v, int n)
{
#ifdef RENKU_NATIVE
	atomic_set(&v->counter, n);
#else
	__atomic_store_n(&v->counter, n, __ATOMIC_RELEASE);
#endif
}
static inline int renku_linux_atomic_read(const atomic_t *v)
{
#ifdef RENKU_NATIVE
	/* Native atomic_get's public prototype lacks const, but only loads. */
	return atomic_get((int32 *)&v->counter);
#else
	return __atomic_load_n(&v->counter, __ATOMIC_ACQUIRE);
#endif
}
static inline int renku_linux_fetch_add(int n, atomic_t *v)
{
#ifdef RENKU_NATIVE
	return atomic_add(&v->counter, n);
#else
	return __atomic_fetch_add(&v->counter, n, __ATOMIC_SEQ_CST);
#endif
}
static inline int renku_linux_fetch_sub(int n, atomic_t *v)
{
	/* Same compiler primitive as SupportDefs; avoid negating INT_MIN. */
	return __atomic_fetch_sub(&v->counter, n, __ATOMIC_RELEASE);
}
static inline bool renku_linux_try_cmpxchg(atomic_t *v, int *old, int n)
{
#ifdef RENKU_NATIVE
	int previous = atomic_test_and_set(&v->counter, n, *old);
	if (previous == *old) return true;
	*old = previous;
	return false;
#else
	return __atomic_compare_exchange_n(&v->counter, old, n, false,
		__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#endif
}
typedef struct { int64_t counter; } atomic_long_t;
#define ATOMIC_LONG_INIT(n) { .counter = (n) }
static inline long atomic_long_inc_return_relaxed(atomic_long_t *v)
{
#ifdef RENKU_NATIVE
	int64_t previous = atomic_add64(&v->counter, 1);
#else
	int64_t previous = __atomic_fetch_add(&v->counter, 1, __ATOMIC_RELAXED);
#endif
	return (long)((uint64_t)previous + 1);
}
/* RenkuOS's pointer-based atomic_set name must remain intact above. */
#define atomic_set renku_linux_atomic_set
#define atomic_read renku_linux_atomic_read
#define atomic_fetch_add_relaxed renku_linux_fetch_add
#define atomic_fetch_sub_release renku_linux_fetch_sub
#define renku_cmpxchg32(v, old, n) ({ \
	_Static_assert(__builtin_types_compatible_p(__typeof__(*(old)), int) || \
		__builtin_types_compatible_p(__typeof__(*(old)), unsigned int), \
		"atomic_t comparison must use a 32-bit integer"); \
	renku_linux_try_cmpxchg((v), (int *)(old), (n)); })
#define atomic_try_cmpxchg_relaxed renku_cmpxchg32
#define atomic_try_cmpxchg_release renku_cmpxchg32
#define smp_acquire__after_ctrl_dep() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#endif
