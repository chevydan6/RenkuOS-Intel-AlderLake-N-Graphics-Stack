// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_EXECUTION_REPORT_H
#define NATIVE_GPU_EXECUTION_REPORT_H
#include <stdint.h>
#define NATIVE_GPU_EXECUTION_IOCTL UINT32_C(0x91504e02)
// Fixed-width report only; never includes kernel virtual/physical addresses.
struct native_gpu_execution_report {
	uint32_t version, stage;
	int32_t status, mapping, dma, tables, bootstrap, group, reset, engine, csb;
	uint32_t identity, revision, pages, retained, load_attempted;
	uint32_t nonce, observed_nonce, observed_complement, sequence, observed_sequence;
	uint32_t copy_words, copy_matched, source_hash, destination_hash;
	uint32_t mode, error_register, fault_register, ring_tail;
	uint32_t engine_stage, engine_error, csb_error, reset_stage, reset_error;
	uint32_t bootstrap_stage, bootstrap_error, group_stage, group_error;
	uint32_t safety_reason, safety_phase, safety_domain;
	uint32_t pci_result, expected_command, observed_command, expected_status, observed_status;
	uint32_t expected_pm, observed_pm, group_domain, group_reads, group_writes;
	uint32_t csb_tail_raw;
	uint64_t csb_words[4];
	uint64_t activation, completion;
};
#endif
