/* SPDX-License-Identifier: MIT */
#ifndef RENKU_KPI_BASE_H
#define RENKU_KPI_BASE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#ifdef RENKU_NATIVE
#include <KernelExport.h>
#else
extern void panic(const char *, ...);
#endif
typedef uint8_t u8;
typedef uint32_t u32;
/* Linux u64 is unsigned long long on x86-64 (including printf formats). */
typedef unsigned long long u64;
typedef u64 __u64;
typedef long long s64;
#ifndef RENKU_NATIVE
typedef u64 phys_addr_t;
#endif
typedef u64 resource_size_t;
/* Sparse address-space annotation; mapping/access semantics remain unported. */
#define __iomem
#define __force
#define __rcu
struct list_head { struct list_head *next, *prev; };
struct hlist_node { struct hlist_node *next, **pprev; };
struct hlist_head { struct hlist_node *first; };
_Static_assert(sizeof(unsigned long) == 8, "This experimental port targets x86-64");
#ifndef __always_inline
#define __always_inline inline __attribute__((always_inline))
#endif
#define __aligned(x) __attribute__((aligned(x)))
#define __maybe_unused __attribute__((unused))
#define __must_check __attribute__((warn_unused_result))
#define __cond_acquires(x)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define READ_ONCE(x) __atomic_load_n(&(x), __ATOMIC_RELAXED)
#define WRITE_ONCE(x, v) __atomic_store_n(&(x), (v), __ATOMIC_RELAXED)
#define smp_store_release(p, v) __atomic_store_n((p), (v), __ATOMIC_RELEASE)
#define smp_load_acquire(p) __atomic_load_n((p), __ATOMIC_ACQUIRE)
#define container_of(p, t, m) ((t *)((char *)(p) - offsetof(t, m)))
#define container_of_const(p, t, m) container_of(p, t, m)
#define BUILD_BUG_ON_INVALID(x) ((void)sizeof((long)(x)))
#define BUG_ON(x) do { if (x) panic("LinuxKPI assertion: %s", #x); } while (0)
extern bool renku_kpi_warn(const char *, ...);
#define WARN(c, ...) ({ bool _warn = !!(c); if (_warn) renku_kpi_warn(__VA_ARGS__); _warn; })
#define WARN_ON(c) WARN(c, "i915 LinuxKPI warning: %s\n", #c)
#define WARN_ONCE(c, ...) ({ bool _condition = !!(c); \
	static bool _warned; \
	if (_condition && !__atomic_exchange_n(&_warned, true, __ATOMIC_RELAXED)) \
		renku_kpi_warn(__VA_ARGS__); \
	_condition; })
#define EXPORT_SYMBOL(x)
#define EXPORT_SYMBOL_GPL(x)
/* Only debug-list=off is supported; debug validators are not simulated. */
#if defined(CONFIG_DEBUG_LIST) && CONFIG_DEBUG_LIST != 0
#error CONFIG_DEBUG_LIST validation backend is not ported
#endif
#ifdef CONFIG_DRM_DEBUG_MM
#error CONFIG_DRM_DEBUG_MM stack-depot backend is not ported
#endif
#ifndef CONFIG_DEBUG_LIST
#define CONFIG_DEBUG_LIST 0
#endif
#define IS_ENABLED(x) (x)
#define LIST_POISON1 ((void *)0x100)
#define LIST_POISON2 ((void *)0x122)
#define U64_MAX (~0ULL)
#define BIT(n) (1UL << (n))
#define lower_32_bits(n) ((u32)(n))
/* Linux errno namespace, deliberately separate from RenkuOS status_t. */
#undef ENOSPC
#define ENOSPC 28
#undef min
#undef max
#define min(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b; })
#define max(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })
static inline bool is_power_of_2(unsigned long long x) { return x && !(x & (x - 1)); }
static inline bool test_bit(unsigned int bit, const unsigned long *p)
{ return !!(__atomic_load_n(p + bit / 64, __ATOMIC_RELAXED) & (1UL << (bit % 64))); }
static inline void __set_bit(unsigned int bit, unsigned long *p)
{ p[bit / 64] |= 1UL << (bit % 64); }
static inline void __clear_bit(unsigned int bit, unsigned long *p)
{ p[bit / 64] &= ~(1UL << (bit % 64)); }
static inline void clear_bit_unlock(unsigned int bit, unsigned long *p)
{ __atomic_fetch_and(p + bit / 64, ~(1UL << (bit % 64)), __ATOMIC_RELEASE); }
static inline u64 div64_u64_rem(u64 n, u64 d, u64 *r)
{ *r = n % d; return n / d; }
/* Tree RCU publication only; no grace-period or reclamation API is provided. */
#define rcu_assign_pointer(p, v) smp_store_release(&(p), (v))
#endif
