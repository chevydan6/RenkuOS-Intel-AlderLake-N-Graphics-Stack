// SPDX-License-Identifier: MIT
// Copyright © 2014 Intel Corporation
// Native adaptation Copyright (c) 2026 chevydan6
#ifndef NATIVE_GPU_RCS_CONTEXT_IMAGE_H
#define NATIVE_GPU_RCS_CONTEXT_IMAGE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace NativeGpu {
class RcsContextImage {
public:
 struct Input {
  uint32_t identity, revision;
  uint64_t contextGgtt, ringGgtt, pml4Dma;
  uint32_t ringBytes, tail, rpcs;
 };
 static constexpr size_t kBytes = 65536;
 // Fresh Gen12.0 N100 RCS0 image (primary GT/GSI=0), not a saved default
 // context. Linux v6.6 intel_lrc.c set_offsets/gen12_rcs_offsets,
 // __lrc_init_regs/lrc_update_regs and gen12_emit_indirect_ctx_rcs;
 // gen8_engine_cs.c gen12_emit_aux_table_inv. Intel MIT logic retained.
 // Caller proves RPCS/topology, ownership, pinned mappings, DMA admission,
 // PAT/MOCS, engine/context WA and default-state bootstrap before submission.
 // This CPU-only constructor establishes none of those hardware contracts.
 static bool Build(const Input& input, uint32_t* output, size_t bytes)
 {
  const Input in = input; // Input may reside inside the destination.
  const uintptr_t cpu = reinterpret_cast<uintptr_t>(output);
  if (!output || bytes != kBytes || (cpu & 3)
   || cpu > UINTPTR_MAX - (kBytes - 1)
   || in.identity != 0x46d18086 || in.revision != 0
   || (in.contextGgtt & 4095) || in.contextGgtt > (1ULL<<32)-kBytes
   || in.ringBytes < 4096 || in.ringBytes > (1U<<21)
   || (in.ringBytes & (in.ringBytes-1))
   || (in.ringGgtt & 4095) || in.ringGgtt > (1ULL<<32)-in.ringBytes
   || (in.contextGgtt < in.ringGgtt + in.ringBytes
    && in.ringGgtt < in.contextGgtt + kBytes)
   || (in.pml4Dma & 4095) || in.pml4Dma > (1ULL<<39)-4096
   || !in.tail || (in.tail & 7) || in.tail > in.ringBytes-64)
   return false;
  // All validation precedes writes: HWSP + 13 HW context pages, then WA
  // and a separately terminating per-context batch. No backing aliases.
  memset(output, 0, kBytes);
  uint32_t* regs = output + 1024;
  static const uint32_t first[] = {0x244,0x034,0x030,0x038,0x03c,
   0x168,0x140,0x110,0x1c0,0x1c4,0x1c8,0x180,0x2b4};
  static const uint32_t second[] = {0x3a8,0x28c,0x288,0x284,0x280,
   0x27c,0x278,0x274,0x270};
  static const uint32_t third[] = {0x1b0,0x5a8,0x5ac};
  static const uint32_t fourth[] = {
   0x588,0x588,0x588,0x588,0x588,0x588,0x028,0x09c,0x0c0,
   0x178,0x17c,0x358,0x170,0x150,0x154,0x158,0x41c,
   0x600,0x604,0x608,0x60c,0x610,0x614,0x618,0x61c,
   0x620,0x624,0x628,0x62c,0x630,0x634,0x638,0x63c,
   0x640,0x644,0x648,0x64c,0x650,0x654,0x658,0x65c,
   0x660,0x664,0x668,0x66c,0x670,0x674,0x678,0x67c,0x068,0x084};
  regs[1] = 0x11081019;
  for (unsigned i=0; i<13; ++i) regs[2+2*i] = 0x2000 + first[i];
  regs[0x21] = 0x11081011;
  for (unsigned i=0; i<9; ++i) regs[0x22+2*i] = 0x2000 + second[i];
  regs[0x34] = 0x11081005;
  for (unsigned i=0; i<3; ++i) regs[0x35+2*i] = 0x2000 + third[i];
  regs[0x41] = 0x11080001;
  regs[0x42] = 0x20c8;
  regs[0x51] = 0x11081065;
  for (unsigned i=0; i<51; ++i) regs[0x52+2*i] = 0x2000 + fourth[i];
  regs[0xb9] = 0x05000001; // set_offsets(close), Gen11+ bit0.
  regs[3] = 0x00090009; // Inhibit synchronous switch and fresh restore.
  regs[7] = in.tail;
  regs[9] = uint32_t(in.ringGgtt);
  regs[11] = (in.ringBytes-4096) | 1;
  regs[0x31] = uint32_t(in.pml4Dma >> 32);
  regs[0x33] = uint32_t(in.pml4Dma);
  regs[0x43] = in.rpcs;
  regs[0x61] = 0x01000000; // Masked STOP_RING clear.
  const uint32_t context = uint32_t(in.contextGgtt);
  const uint32_t indirect = context + 14*4096;
  regs[0x13] = (context + 15*4096) | 5; // PER_CTX_BB force + valid.
  regs[0x15] = indirect | 2; // 128 bytes, two cachelines.
  regs[0x17] = 0x0d << 6;
  uint32_t* wa = output + 14*1024;
  const uint32_t commands[] = {
   0x14c80002,0x600,context+4096+0x23*4,0,
   0x150c0001,0x600,0x3a8, 0x150c0001,0x600,0x3a8,
   0x14c80002,0x600,context+4096+0xb7*4,0,
   0x150c0001,0x600,0x084,
   0x14c80002,0x600,context+4096+0x75*4,0,
   0x11020001,0x4208,1, // RCS AUX invalidation (N100 has no flat CCS).
   0x0e01c003,0,0x4208,0,0
  };
  memcpy(wa, commands, sizeof commands); // 29 dwords, padded by zero NOOPs.
  // setup_indirect_ctx_bb also installs this dormant predicate helper,
  // outside the 128-byte indirect span; it is not linked by this builder.
  const uint32_t predicate[] = {
   0x10400002,indirect+4096-8,0,0,0x05008000,0x00800000,
   0x10400002,indirect+4096-8,0,1,0x05000000
  };
  memcpy(wa+512, predicate, sizeof predicate);
  output[15*1024] = 0x05000000;
  return true;
 }
};
}
#endif
