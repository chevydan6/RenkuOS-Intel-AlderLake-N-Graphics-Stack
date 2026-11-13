/* SPDX-License-Identifier: MIT */
#ifndef RENKU_VMA_TEST_H
#define RENKU_VMA_TEST_H
struct vma_report { unsigned checks, failures, first_line; };
struct vma_native_report {
	struct vma_report result;
	unsigned version, iterations, max_readers, worker_errors;
};
#define VMA_IOCTL (B_DEVICE_OP_CODES_END + 123)
#endif
