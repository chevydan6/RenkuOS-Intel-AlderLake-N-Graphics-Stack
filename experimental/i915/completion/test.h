/* SPDX-License-Identifier: MIT */
#ifndef RENKU_COMPLETION_TEST_H
#define RENKU_COMPLETION_TEST_H
struct completion_report { unsigned version, checks, failures, first_line, phases, workers, cpu_count, online_count, bit_workers, bit_iterations, rcu_workers, rcu_frees, lockref_workers, lockref_iterations, seq_workers, seq_writes, seq_reads, seq_forced_retries, page_allocations, page_frees, page_workers, page_iterations, store_allocations, store_frees, store_workers, store_iterations; unsigned long online_bits; };
_Static_assert(sizeof(unsigned) == 4 && sizeof(unsigned long) == 8 && sizeof(struct completion_report) == 112,
	"version9 diagnostic report requires the x86-64 kernel/client ABI");
struct completion_signal_report { long result; unsigned queued; };
#define COMPLETION_RUN (B_DEVICE_OP_CODES_END + 130)
#define COMPLETION_INTERRUPTIBLE (COMPLETION_RUN + 1)
#define COMPLETION_KILLABLE (COMPLETION_RUN + 2)
#define COMPLETION_QUEUED (COMPLETION_RUN + 3)
#define COMPLETION_SIGNAL (COMPLETION_RUN + 4)
#endif
