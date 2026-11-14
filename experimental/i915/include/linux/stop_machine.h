/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_STOP_MACHINE_H
#define RENKU_LINUX_STOP_MACHINE_H
#include <linux/cpumask.h>
typedef int (*cpu_stop_fn_t)(void *);
/* All-CPU exclusion is a native service still to be implemented. Do not pull
 * Linux CPU-device/process-mm implementation headers into its callers, or
 * substitute a callback invocation without the required global exclusion. */
int stop_machine(cpu_stop_fn_t, void *, const struct cpumask *);
int stop_machine_cpuslocked(cpu_stop_fn_t, void *, const struct cpumask *);
#endif
