/* SPDX-License-Identifier: MIT */
#ifndef RENKU_NATIVE_CPUMASK_H
#define RENKU_NATIVE_CPUMASK_H
#include <linux/cpumask.h>
/* Serialized, fixed-topology module lifecycle. No new mask users may enter
 * before publication or after reset; callers exclude CPU enable/disable changes.
 * This API does not provide Linux CPU hotplug, NUMA placement or affinity. */
int renku_linux_cpumask_publish(unsigned count, unsigned long enabled);
void renku_linux_cpumask_reset(void);
#ifdef RENKU_NATIVE
status_t renku_linux_cpu_backend_init(void);
void renku_linux_cpu_backend_uninit(void);
#endif
#endif
