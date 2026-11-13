/* SPDX-License-Identifier: MIT */
/* Intentionally no include guard: each inclusion must check its caller. This
 * supplies NO generated bounds, page flags, zones, or guessed lock sizes. The
 * original lockref header needs none with architecture cmpxchg disabled. */
#if !defined(RENKU_IMPORT_LOCKREF)
#error Linux VM generated bounds are not supplied by the native lockref adapter
#endif
#if defined(CONFIG_ARCH_USE_CMPXCHG_LOCKREF) && CONFIG_ARCH_USE_CMPXCHG_LOCKREF
#error Native lockref supports only the original spinlock fallback
#endif
