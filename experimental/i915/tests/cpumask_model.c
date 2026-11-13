/* SPDX-License-Identifier: MIT */
/* Actual adapter and original helpers, modeled CPU-query services only.
 * Tests sparse enabled CPU IDs without changing physical CPU state. */
#include <native_cpumask.h>
#include <linux/errno.h>
static int32 count;
static unsigned queries;
static bool change_count;
static unsigned long enabled;
static status_t query_status;
int32 smp_get_num_cpus(void) { return count + (change_count && queries++ > 0); }
status_t _get_cpu_info_etc(uint32 first, uint32 n, cpu_info *info, size_t size)
{
	if (first || n != (uint32)count || size != sizeof(*info)) __builtin_trap();
	if (query_status != B_OK) return query_status;
	memset(info, 0, n * sizeof(*info));
	for (unsigned cpu = 0; cpu < n; cpu++) info[cpu].enabled = (enabled >> cpu) & 1;
	return B_OK;
}
#define EXPECT(x) do { if (!(x)) return __LINE__; } while (0)
int main(void)
{
	EXPECT(!nr_cpu_ids && !num_online_cpus() && cpumask_empty(cpu_possible_mask));
	EXPECT(renku_linux_cpumask_publish(0, 1) == -EINVAL);
	EXPECT(renku_linux_cpumask_publish(65, 1) == -EINVAL);
	EXPECT(renku_linux_cpumask_publish(4, 0) == -EINVAL);
	EXPECT(renku_linux_cpumask_publish(4, 0x10) == -EINVAL);
	count = 0; EXPECT(renku_linux_cpu_backend_init() == B_BAD_VALUE);
	count = 65; EXPECT(renku_linux_cpu_backend_init() == B_BAD_VALUE);
	count = 4; enabled = 5; query_status = B_TIMED_OUT;
	EXPECT(renku_linux_cpu_backend_init() == B_TIMED_OUT && nr_cpu_ids == 0);
	query_status = B_OK; change_count = true;
	EXPECT(renku_linux_cpu_backend_init() == B_BUSY && nr_cpu_ids == 0);
	change_count = false;
	EXPECT(renku_linux_cpu_backend_init() == B_OK);
	EXPECT(renku_linux_cpu_backend_init() == B_BUSY);
	/* Stable negative-control code for incorrectly admitting disabled CPUs. */
	if (num_online_cpus() != 2) return 90;
	EXPECT(nr_cpu_ids == 4 && num_possible_cpus() == 4);
	EXPECT(cpu_online(0) && !cpu_online(1) && cpu_online(2) && !cpu_online(3));
	EXPECT(cpu_present(1) && cpu_present(3) && !cpu_present(4));
	EXPECT(cpumask_equal(cpu_active_mask, cpu_online_mask));
	unsigned cpu, seen = 0;
	for_each_online_cpu(cpu) { EXPECT(cpu == (seen ? 2 : 0)); seen++; }
	EXPECT(seen == 2);
	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		EXPECT(cpumask_weight(cpumask_of(cpu)) == 1);
		EXPECT(cpumask_test_cpu(cpu, cpumask_of(cpu)));
	}
	renku_linux_cpu_backend_uninit();
	EXPECT(!nr_cpu_ids && !num_online_cpus() && cpumask_empty(cpu_online_mask));
	count = 64; enabled = (1UL << 63) | 1;
	EXPECT(renku_linux_cpu_backend_init() == B_OK && nr_cpu_ids == 64);
	EXPECT(num_online_cpus() == 2 && cpu_online(63) && !cpu_online(62));
	EXPECT(cpumask_last(cpu_online_mask) == 63 && num_possible_cpus() == 64);
	renku_linux_cpu_backend_uninit();
	count = 1; enabled = 1;
	EXPECT(renku_linux_cpu_backend_init() == B_OK && num_online_cpus() == 1);
	renku_linux_cpu_backend_uninit();
	enabled = 0;
	EXPECT(renku_linux_cpu_backend_init() == B_BAD_VALUE && !nr_cpu_ids);
	return 0;
}
