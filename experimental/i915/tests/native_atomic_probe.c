/* SPDX-License-Identifier: MIT */
/* Compile with RENKU_NATIVE and real target headers; no kernel calls needed. */
#include <linux/atomic.h>
enum {
	native_overflow = B_BUFFER_OVERFLOW, native_args = B_TOO_MANY_ARGS,
	native_file_size = B_FILE_TOO_LARGE, native_range = B_RESULT_NOT_REPRESENTABLE,
	native_device = B_DEVICE_NOT_FOUND, native_unsupported = B_NOT_SUPPORTED
};
#include <linux/err.h>
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
	atomic_long_t stamp = ATOMIC_LONG_INIT(LONG_MAX);
	if (atomic_long_inc_return_relaxed(&stamp) != LONG_MIN) return __LINE__;
	if (atomic_long_inc_return_relaxed(&stamp) != LONG_MIN + 1) return __LINE__;
	if (EINVAL != 22 || EDEADLK != 35 || ENOSPC != 28) return __LINE__;
	if ((int)B_BUFFER_OVERFLOW != (int)native_overflow || (int)B_TOO_MANY_ARGS != (int)native_args ||
		(int)B_FILE_TOO_LARGE != (int)native_file_size || (int)B_RESULT_NOT_REPRESENTABLE != (int)native_range ||
		(int)B_DEVICE_NOT_FOUND != (int)native_device || (int)B_NOT_SUPPORTED != (int)native_unsupported)
		return __LINE__;
	for (long error = 1; error <= MAX_ERRNO; ++error) {
		void *pointer = ERR_PTR(-error);
		if (!IS_ERR(pointer) || !IS_ERR_OR_NULL(pointer)) return __LINE__;
		if (PTR_ERR(pointer) != -error || PTR_ERR_OR_ZERO(pointer) != -error) return __LINE__;
		if (ERR_CAST(pointer) != pointer) return __LINE__;
	}
	if (IS_ERR(&v) || IS_ERR(NULL) || !IS_ERR_OR_NULL(NULL)) return __LINE__;
	if (PTR_ERR_OR_ZERO(&v) != 0) return __LINE__;
	return 0;
}
