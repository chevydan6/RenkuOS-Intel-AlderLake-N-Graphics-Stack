/* SPDX-License-Identifier: MIT */
#include <native_cpumask.h>
#ifndef RENKU_NATIVE
#error This adapter requires actual native CPU information
#endif
extern int32 smp_get_num_cpus(void);
status_t renku_linux_cpu_backend_init(void)
{
	if (nr_cpu_ids) return B_BUSY;
	int32 count = smp_get_num_cpus();
	if (count <= 0 || count > NR_CPUS) return B_BAD_VALUE;
	cpu_info info[NR_CPUS];
	status_t status = get_cpu_info(0, count, info);
	if (status != B_OK) return status;
	unsigned long enabled = 0;
	for (int32 cpu = 0; cpu < count; cpu++)
		if (info[cpu].enabled) enabled |= 1UL << cpu;
	if (smp_get_num_cpus() != count) return B_BUSY;
	/* Module owner excludes topology/enabled-state changes for this lifetime. */
	if (renku_linux_cpumask_publish(count, enabled) != 0) return B_BAD_VALUE;
	return B_OK;
}
void renku_linux_cpu_backend_uninit(void) { renku_linux_cpumask_reset(); }
