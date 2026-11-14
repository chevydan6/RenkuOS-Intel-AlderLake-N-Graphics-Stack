// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_BUFFER_COPY_H
#define NATIVE_GPU_BUFFER_COPY_H
#include "ExecutionReport.h"
#define NATIVE_GPU_BUFFER_CREATE UINT32_C(0x91504e10)
#define NATIVE_GPU_BUFFER_WRITE UINT32_C(0x91504e11)
#define NATIVE_GPU_BUFFER_READ UINT32_C(0x91504e12)
#define NATIVE_GPU_BUFFER_COPY UINT32_C(0x91504e13)
static constexpr uint32_t kNativeBufferBytes=4096;
// Handles live in the open-file session's namespace, never GPU addresses.
// This first ABI supports two fixed-size buffers, origin-to-origin copies,
// positive 32-bit linear pixels, independent pitches, and synchronous polling.
struct native_gpu_buffer_create { uint32_t bytes,handle; };
struct native_gpu_buffer_io {
 uint32_t handle,offset,bytes;
 uint8_t data[kNativeBufferBytes];
};
struct native_gpu_copy {
 uint32_t source,destination,width,height,source_pitch,destination_pitch;
 native_gpu_execution_report report;
};
#endif
