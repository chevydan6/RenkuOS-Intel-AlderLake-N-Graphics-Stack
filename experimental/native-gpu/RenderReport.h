// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RENDER_REPORT_H
#define NATIVE_GPU_RENDER_REPORT_H
#include <stdint.h>
#define NATIVE_GPU_RENDER_IOCTL UINT32_C(0x91504e30)
#define NATIVE_GPU_RENDER_LARGE_IOCTL UINT32_C(0x91504e32)
enum { NATIVE_GPU_RENDER_PROBE=1, NATIVE_GPU_RENDER_BOOTSTRAP=2, NATIVE_GPU_RENDER_DRAW=3 };
enum { NATIVE_GPU_RENDER_NO_HANDOFF=1, NATIVE_GPU_RENDER_NEEDS_BOOTSTRAP=2,
 NATIVE_GPU_RENDER_ALREADY_BOOTSTRAPPED=3, NATIVE_GPU_RENDER_SEQUENCE_EXHAUSTED=4,
 NATIVE_GPU_RENDER_RESOURCE_UNAVAILABLE=5 };
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
 // Version 2 or 3 + DRAW + variant 0: the first 48 bytes initially contain three
 // position vec4s. The owner validates/copies them before zeroing this array;
 // on return the full array is the rendered 32x32 BGRA image, as in version 1.
 // Version 3 uses a separate owner-allocated GPU vertex page. No user GPU
 // address, command word, shader or surface state is accepted.
 // Version 4: reserved is a file-local vertex BO handle. Version 5: variant
 // is a vertex BO handle and reserved is a batch BO handle. The owner validates
 // every batch dword against one known-safe template before GPU submission.
 // Version 6: variant selects fixed triangle 0..3; reserved is a file-local
 // fragment shader BO handle. The owner validates its entire 4 KiB page
 // against the proven shader image before widening the instruction window.
 // Version 7: variant is a file-local vertex BO handle; reserved is a
 // different file-local fragment shader BO handle. Both pages and the
 // combined fixed batch are validated before GPU publication.
 // Version 8: version 7 plus a third, distinct file-local vertex shader BO
 // handle in output[0..3] on input. The owner validates its complete page,
 // copies the approved instructions into its private VS page, and overwrites
 // output with rendered pixels on success.
 uint8_t output[4096];
};
// Separate ioctl: version 9 returns a fixed 64x64 BGRA image. Version 10
// uses version-7's file-local vertex and fragment handles with the 64x64
// target. Version 11 adds version-8's distinct vertex shader handle in the
// first four output bytes on input. Version 12 uses version-10's handles
// with the exact FragCoord-red fragment program and compiler-matched PS state.
// Version 13 uses version-7/10 handles for either target size, with a nonzero
// 16-byte-aligned vertex offset in the first four output bytes on input.
// The owner validates the selected three-vertex range before GPU publication.
// Version 14 adds a distinct owner-approved vertex shader handle in input
// output bytes 4..7 and requires a constant-color fragment shader.
// Version 15 renders directly to an owner-allocated image resource. Input
// output bytes 0..3 hold a possibly zero vertex offset, bytes 4..7 an
// optional approved vertex shader handle, and bytes 8..11 the distinct image
// resource handle. The image resource must cover the complete 32/64 target.
// Version 16 extends version 15 with a distinct 16-byte uniform-buffer
// resource handle in input output bytes 12..15. The fragment shader BO must
// contain the exact N100 set0/binding0 UBO program; the owner resolves the
// uniform BO, builds binding-table entry 1, and verifies the rendered color.
// The v1-v8 4 KiB ABI remains unchanged.
// No user GPU address or packet word is accepted.
struct native_gpu_render_large_command {
 native_gpu_render_command base;
 uint8_t extra[12288];
};
static_assert(sizeof(native_gpu_render_report)==240,"render report ABI");
static_assert(sizeof(native_gpu_render_command)==4352,"render command ABI");
static_assert(sizeof(native_gpu_render_large_command)==16640,"large render command ABI");
#endif
