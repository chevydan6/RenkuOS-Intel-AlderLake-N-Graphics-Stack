// SPDX-License-Identifier: MIT
#include "RcsContextImage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <new>
#include "rcs_linux_golden.h"
using NativeGpu::RcsContextImage;
static const RcsContextImage::Input valid = {0x46d18086, 0, 0x10000, 0x30000,
 0x12345000, 4096, 96, 0x80000000};
static void rejected(RcsContextImage::Input in, size_t bytes = 65536)
{
 alignas(8) uint32_t output[16384];
 memset(output, 0x5a, sizeof output);
 assert(!RcsContextImage::Build(in, output, bytes));
 for (auto x : output) assert(x == 0x5a5a5a5a);
}
int main()
{
 alignas(8) uint32_t image[16386];
 memset(image, 0x5a, sizeof image);
 assert(RcsContextImage::Build(valid, image + 1, 65536));
 assert(image[0] == 0x5a5a5a5a && image[16385] == 0x5a5a5a5a);
 uint32_t expected[16384] = {};
 memcpy(expected + 1024, kLinuxRcsRegisters, sizeof kLinuxRcsRegisters);
 uint32_t* regs = expected + 1024;
 regs[3] = 0x00090009;
 regs[7] = 96;
 regs[9] = 0x30000;
 regs[11] = 1;
 regs[0x13] = 0x1f005;
 regs[0x15] = 0x1e002;
 regs[0x17] = 0x340;
 regs[0x33] = 0x12345000;
 regs[0x43] = 0x80000000;
 regs[0x61] = 0x01000000;
 // Source-derived WA, independent of implementation; exact 29 dwords + padding.
 const uint32_t wa[] = {
  0x14c80002,0x600,0x1108c,0, 0x150c0001,0x600,0x3a8,
  0x150c0001,0x600,0x3a8,
  0x14c80002,0x600,0x112dc,0, 0x150c0001,0x600,0x84,
  0x14c80002,0x600,0x111d4,0,
  0x11020001,0x4208,1, 0x0e01c003,0,0x4208,0,0
 };
 static_assert(sizeof wa == 29 * 4, "WA length");
 memcpy(expected + 14*1024, wa, sizeof wa);
 const uint32_t predicate[] = {0x10400002,0x1eff8,0,0,0x05008000,0x00800000,
  0x10400002,0x1eff8,0,1,0x05000000};
 memcpy(expected + 14*1024 + 512, predicate, sizeof predicate);
 expected[15*1024] = 0x05000000;
 assert(!memcmp(expected, image + 1, sizeof expected));
 auto in = valid;
 in.identity++; rejected(in); in = valid;
 in.revision = 1; rejected(in); in = valid;
 in.contextGgtt++; rejected(in); in = valid;
 in.contextGgtt = (1ULL<<32)-65536+4096; rejected(in); in = valid;
 in.ringGgtt = 0x18000; rejected(in); in = valid;
 in.ringGgtt++; rejected(in); in = valid;
 in.ringGgtt = 1ULL<<32; rejected(in); in = valid;
 in.ringBytes = 8191; rejected(in); in = valid;
 in.ringBytes = 2048; rejected(in); in = valid;
 in.ringBytes = 1U<<22; rejected(in); in = valid;
 in.pml4Dma++; rejected(in); in = valid;
 in.pml4Dma = 1ULL<<39; rejected(in); in = valid;
 in.tail = 0; rejected(in); in = valid;
 in.tail = 4; rejected(in); in = valid;
 in.tail = 4096-56; rejected(in);
 rejected(valid, 65532); rejected(valid, 65540);
 assert(!RcsContextImage::Build(valid, nullptr, 65536));
 assert(!RcsContextImage::Build(valid, reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(image)+1), 65536));
 in = valid; in.contextGgtt = (1ULL<<32)-65536; in.ringGgtt = 0;
 in.pml4Dma = (1ULL<<39)-4096; in.ringBytes = 1U<<21; in.tail = in.ringBytes-64;
 in.rpcs = 0x12345678; // Preserved as caller-validated data, not validated topology.
 assert(RcsContextImage::Build(in, image, 65536));
 assert(image[1024+0x31] == 0x7f && image[1024+0x33] == 0xfffff000);
 assert(image[1024+0x43] == in.rpcs && image[1024+0x15] == 0xffffe002);
 // Input may alias output: constructor must snapshot it before clearing.
 auto* aliased = new (image) RcsContextImage::Input(valid);
 assert(RcsContextImage::Build(*aliased, image, 65536));
 assert(!memcmp(expected, image, sizeof expected));
 puts("Native RCS context-image smoke: PASS (CPU construction only)");
}
