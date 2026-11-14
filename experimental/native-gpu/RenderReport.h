// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RENDER_REPORT_H
#define NATIVE_GPU_RENDER_REPORT_H
#include <stdint.h>
#define NATIVE_GPU_RENDER_IOCTL UINT32_C(0x91504e30)
enum { NATIVE_GPU_RENDER_PROBE=1, NATIVE_GPU_RENDER_BOOTSTRAP=2, NATIVE_GPU_RENDER_DRAW=3 };
enum { NATIVE_GPU_RENDER_NO_HANDOFF=1, NATIVE_GPU_RENDER_NEEDS_BOOTSTRAP=2,
 NATIVE_GPU_RENDER_ALREADY_BOOTSTRAPPED=3, NATIVE_GPU_RENDER_SEQUENCE_EXHAUSTED=4 };
enum { NATIVE_GPU_RENDER_INITIALIZED=1, NATIVE_GPU_RENDER_DEFAULT_VALID=2,
 NATIVE_GPU_RENDER_POOL_READY=4, NATIVE_GPU_RENDER_STAGE_COMPLETE=12 };
// Exact IntelN100PciPower::Result values; meaningful after Capture is attempted.
// Zero also remains the default before that stage. No retry/relaxed admission.
enum native_gpu_render_pci_error : uint32_t {
 NATIVE_GPU_RENDER_PCI_SUCCESS=0, NATIVE_GPU_RENDER_PCI_INVALID_MAPPING=1,
 NATIVE_GPU_RENDER_PCI_READ_FAILED=2, NATIVE_GPU_RENDER_PCI_UNSUPPORTED_DEVICE=3,
 NATIVE_GPU_RENDER_PCI_MEMORY_DISABLED=4, NATIVE_GPU_RENDER_PCI_INVALID_BAR=5,
 NATIVE_GPU_RENDER_PCI_INVALID_CAPABILITIES=6, NATIVE_GPU_RENDER_PCI_MISSING_POWER=7,
 NATIVE_GPU_RENDER_PCI_NOT_D0=8, NATIVE_GPU_RENDER_PCI_UNSTABLE=9
};
// Kernel-built workloads only. No addresses, register offsets or GPU commands.
struct native_gpu_render_report {
 uint32_t version, stage;
 int32_t status;
 uint32_t mode, variant, refusal, sequence;
 uint32_t identity, revision, known_profile, owner_granted, retained;
 uint32_t slice_enable, geometry_dss_enable, eu_disable, media_disable, mirror_fuse3;
 uint32_t slice_mask, dss_mask, eu_mask, eu_count, rpcs;
 uint32_t ring_control, ring_mode, el_status_lo, el_status_hi, eir, esr, fault;
 int32_t mapping, dma, bootstrap, group;
 uint32_t bootstrap_stage, bootstrap_error, group_stage, group_error, safety_reason;
 uint32_t nonce, observed_nonce, observed_complement, observed_sequence;
 uint32_t ring_tail, csb_error, engine_stage, engine_error, reset_stage, reset_error;
 uint64_t activation, completion;
 uint32_t state_flags, checked_pixels, matched_pixels, target_hash;
 uint32_t interior_xy, exterior_xy, csb_cursor, pci_error;
};
struct native_gpu_render_command {
 uint32_t version, mode, variant, reserved;
 native_gpu_render_report report;
 // Version 2 + DRAW + variant 0: the first 48 bytes initially contain three
 // position vec4s. The owner validates/copies them before zeroing this array;
 // on return the full array is the rendered 32x32 BGRA image, as in version 1.
 // No user GPU address, command word, shader or surface state is accepted.
 uint8_t output[4096];
};
static_assert(sizeof(native_gpu_render_report)==240,"render report ABI");
static_assert(sizeof(native_gpu_render_command)==4352,"render command ABI");
#endif
