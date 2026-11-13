/* SPDX-License-Identifier: MIT */
#ifndef I915_MAPPING_NATIVE_PROBE_H
#define I915_MAPPING_NATIVE_PROBE_H
#include <SupportDefs.h>
#define I915_MAPPING_TEST_IOCTL 0x91504d01
struct i915_mapping_result {
	uint32 version, checks, failures;
	status_t module_status, begin_status, end_status;
	uint64 ggtt[4], bytes[4];
};
#endif
