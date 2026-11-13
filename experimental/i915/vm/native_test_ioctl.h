/* SPDX-License-Identifier: MIT */
#ifndef RENKU_I915_NATIVE_TEST_IOCTL_H
#define RENKU_I915_NATIVE_TEST_IOCTL_H
#include <Drivers.h>
#define KPI_TEST_DEVICE "misc/i915_linuxkpi_test"
#define KPI_TEST_PATH "/dev/" KPI_TEST_DEVICE
#define KPI_RUN_SELFTEST (B_DEVICE_OP_CODES_END + 1)
struct kpi_test_report {
	uint32 abi_version;
	uint32 checks, failures, first_failure_line;
	uint32 allocated_pages, freed_pages, worker_iterations, final_releases;
	int32 timeout_status;
	uint32 kernel_threads;
	uint64 timeout_elapsed_us;
	uint64 elapsed_us;
};
#endif
