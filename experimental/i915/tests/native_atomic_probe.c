/* SPDX-License-Identifier: MIT */
/* Compile with RENKU_NATIVE and real target headers; no kernel calls needed. */
#include <linux/atomic.h>
int renku_native_atomic_probe(void)
{
	atomic_t v = ATOMIC_INIT(1);
	if (atomic_read(&v) != 1) return __LINE__;
	atomic_set(&v, INT_MAX);
	if (atomic_fetch_add_relaxed(1, &v) != INT_MAX) return __LINE__;
	if (atomic_read(&v) != INT_MIN) return __LINE__;
	if (atomic_fetch_sub_release(1, &v) != INT_MIN) return __LINE__;
	if (atomic_read(&v) != INT_MAX) return __LINE__;
	int old = 0;
	if (atomic_try_cmpxchg_relaxed(&v, &old, 8)) return __LINE__;
	if (old != INT_MAX || atomic_read(&v) != INT_MAX) return __LINE__;
	if (!atomic_try_cmpxchg_release(&v, &old, 8)) return __LINE__;
	if (old != INT_MAX || atomic_read(&v) != 8) return __LINE__;
	unsigned int unsigned_old = 8;
	if (!atomic_try_cmpxchg_relaxed(&v, &unsigned_old, 9)) return __LINE__;
	if (unsigned_old != 8 || atomic_read(&v) != 9) return __LINE__;
	return 0;
}
