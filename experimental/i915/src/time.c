/* SPDX-License-Identifier: MIT */
#include <linux/ktime.h>
#ifndef RENKU_NATIVE
#include <time.h>
#endif
ktime_t ktime_get(void)
{
#ifdef RENKU_NATIVE
	return system_time_nsecs();
#else
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
		panic("i915 LinuxKPI: CLOCK_MONOTONIC failed");
		__builtin_trap();
	}
	return (ktime_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
}
