// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_LIFECYCLE_REPORT_H
#define NATIVE_GPU_LIFECYCLE_REPORT_H
#include <stdint.h>
#define NATIVE_GPU_LIFECYCLE_STATUS UINT32_C(0x91504e20)
#define NATIVE_GPU_LIFECYCLE_CONTROL UINT32_C(0x91504e21)
enum native_gpu_lifecycle_action : uint32_t {
 NATIVE_GPU_QUIESCE=1, NATIVE_GPU_RESUME=2, NATIVE_GPU_SHUTDOWN=3,
 // Administrative native check: hide CSB observation for one valid copy.
 // Exercises timeout recovery without submitting malformed GPU commands.
 NATIVE_GPU_TEST_TIMEOUT=4
};
struct native_gpu_lifecycle_control {uint32_t version, action;};
struct native_gpu_request_metrics {
 uint64_t total_us, cache_us, wait_us, submit_us, queue_us;
 uint32_t cache_calls, polls, sleeps, recovery_stage, recovery_error, reset_rounds;
};
// Fixed-width snapshot, no addresses. State values are RequestLifecycle::State.
// Counters cover accepted requests only; concurrent rejection is not a failure.
struct native_gpu_lifecycle_report {
 uint32_t version, state, clients, sequence;
 int32_t last_status;
 uint32_t reserved;
 uint64_t generation, completed, failed;
 native_gpu_request_metrics last;
};
#endif
