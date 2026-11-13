/* SPDX-License-Identifier: MIT */
#ifndef RENKU_CPUMASK_CASES_H
#define RENKU_CPUMASK_CASES_H
#include <native_cpumask.h>
#include <linux/cache.h>
#include <asm/param.h>
static void CPUMaskCases(struct completion_report *r)
{
	_Static_assert(NR_CPUS == 64 && sizeof(cpumask_t) == 8, "original mask capacity");
	_Static_assert(IS_ENABLED(CONFIG_SMP) && HZ == 1000 && USER_HZ == 100, "port configuration");
	_Static_assert(U32_MAX == 0xffffffffU && S32_MIN == (-2147483647 - 1), "original integer limits");
	_Static_assert(L1_CACHE_BYTES == 64 && SMP_CACHE_BYTES == 64, "cache alignment target");
	CHECK(r, nr_cpu_ids > 0 && nr_cpu_ids <= NR_CPUS);
	CHECK(r, num_possible_cpus() == nr_cpu_ids && num_present_cpus() == nr_cpu_ids);
	CHECK(r, num_online_cpus() > 0 && num_online_cpus() <= nr_cpu_ids);
	CHECK(r, num_active_cpus() == num_online_cpus() && cpumask_empty(cpu_dying_mask));
	CHECK(r, cpumask_equal(cpu_possible_mask, cpu_present_mask));
	CHECK(r, cpumask_equal(cpu_online_mask, cpu_active_mask));
#ifdef RENKU_NATIVE
	extern int32 smp_get_num_cpus(void);
	cpu_info native[NR_CPUS];
	CHECK(r, smp_get_num_cpus() == (int32)nr_cpu_ids);
	status_t status = get_cpu_info(0, nr_cpu_ids, native);
	CHECK(r, status == B_OK);
	if (status != B_OK) return;
	unsigned eax, ebx, ecx, edx;
	__asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "0"(1), "2"(0));
	CHECK(r, (edx & (1U << 19)) && ((ebx >> 8) & 0xff) * 8 == L1_CACHE_BYTES);
	(void)eax; (void)ecx;
#endif
	unsigned cpu, seen = 0, enabled = 0;
	for_each_possible_cpu(cpu) {
		CHECK(r, cpu == seen && cpu_possible(cpu) && cpu_present(cpu));
#ifdef RENKU_NATIVE
		CHECK(r, cpu_online(cpu) == native[cpu].enabled);
		enabled += native[cpu].enabled;
#else
		CHECK(r, cpu_online(cpu) == ((0x5UL >> cpu) & 1));
		enabled += (0x5UL >> cpu) & 1;
#endif
		seen++;
	}
	CHECK(r, seen == nr_cpu_ids && enabled == num_online_cpus());
	seen = 0;
	for_each_online_cpu(cpu) { CHECK(r, cpu < nr_cpu_ids && cpu_online(cpu)); seen++; }
	CHECK(r, seen == num_online_cpus());
	cpumask_var_t local;
	CHECK(r, zalloc_cpumask_var(&local, GFP_KERNEL));
	CHECK(r, cpumask_empty(local) && cpumask_size() == sizeof(cpumask_t));
	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		const struct cpumask *one = cpumask_of(cpu);
		CHECK(r, cpumask_weight(one) == 1 && cpumask_first(one) == cpu);
		cpumask_set_cpu(cpu, local);
		CHECK(r, cpumask_test_cpu(cpu, local) && cpumask_weight(local) == cpu + 1);
	}
	CHECK(r, cpumask_full(local));
	cpumask_clear(local);
	cpumask_set_cpu(0, local);
	CHECK(r, cpumask_test_and_set_cpu(0, local));
	CHECK(r, cpumask_test_and_clear_cpu(0, local) && cpumask_empty(local));
	cpumask_copy(local, cpu_online_mask);
	CHECK(r, cpumask_equal(local, cpu_online_mask));
	cpumask_t combined;
	CHECK(r, cpumask_and(&combined, local, cpu_present_mask));
	CHECK(r, cpumask_equal(&combined, local) && cpumask_subset(local, cpu_present_mask));
	cpumask_xor(&combined, local, local); CHECK(r, cpumask_empty(&combined));
	cpumask_or(&combined, cpu_present_mask, local); CHECK(r, cpumask_equal(&combined, cpu_present_mask));
	CHECK(r, !cpumask_andnot(&combined, local, cpu_present_mask) && cpumask_empty(&combined));
	free_cpumask_var(local); /* original inline storage; no heap allocation */
	atomic_t count = ATOMIC_INIT(7);
	int expected = 7;
	CHECK(r, raw_atomic_read(&count) == 7 && atomic_try_cmpxchg(&count, &expected, 3));
	expected = 7;
	CHECK(r, !atomic_try_cmpxchg(&count, &expected, 9) && expected == 3);
	atomic_dec(&count); CHECK(r, atomic_read(&count) == 2);
	CHECK(r, likely_notrace(1) && !unlikely_notrace(0));
	r->cpu_count = nr_cpu_ids; r->online_count = num_online_cpus();
	r->online_bits = cpu_online_mask->bits[0];
}
#endif
