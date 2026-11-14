// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_PREPARE_REPORT_H
#define NATIVE_GPU_PREPARE_REPORT_H
#include <SupportDefs.h>
#define NATIVE_GPU_PREPARE_IOCTL 0x91504e01
struct native_gpu_prepare_report {
	uint32 version;
	status_t mapping, tables, image, cleanup;
	uint32 pages, ring_tail, workaround_tail, ring_checksum, context_checksum;
	uint32 result_poisoned, gpu_submitted;
};
#endif
