// SPDX-License-Identifier: MIT
#include "IntelN100ContextImage.h"
#include <assert.h>
#include <initializer_list>
#include <stdio.h>
#include <string.h>
#include "lrc-reference.inc"

using Image = IntelN100ContextImage;
static unsigned cases;
static void Compare(const Image::Input& input)
{
	alignas(4096) uint32_t guarded[4096 + 32];
	alignas(4096) uint32_t expected[4096];
	for (auto& word : guarded) word = 0xfeed1234;
	memset(expected, 0x5a, sizeof(expected));
	assert(Image::Build(input, guarded + 16, Image::kBytes));
	LinuxReference::Build(input, expected);
	for (unsigned i = 0; i < 4096; i++) {
		if (guarded[16 + i] != expected[i])
			fprintf(stderr, "image dword%#x: actual=%#x reference=%#x\n", i, guarded[16 + i], expected[i]);
	}
	assert(memcmp(guarded + 16, expected, Image::kBytes) == 0);
	for (unsigned i = 0; i < 16; i++) {
		assert(guarded[i] == 0xfeed1234);
		assert(guarded[4112 + i] == 0xfeed1234);
	}
	cases++;
}
static void Reject(const Image::Input& input)
{
	alignas(64) uint32_t output[4096];
	for (auto& word : output) word = 0xfeed1234;
	assert(!Image::Build(input, output, sizeof(output)));
	for (auto word : output) assert(word == 0xfeed1234);
	cases++;
}
int main()
{
	Image::Input base = {0x46d18086, 0, 0x2000000, 0x4000000, 0x123456000, 4096, 64};
	Compare(base);
	for (uint32_t size = 4096; size <= (1U << 21); size *= 2) {
		for (uint32_t tail : {8u, 64u, size / 2, size - 64}) {
			for (uint64_t root : {0ULL, 4096ULL, 0x123456000ULL, (1ULL << 39) - 4096}) {
				auto input = base; input.ringBytes = size; input.tail = tail; input.pml4Dma = root;
				Compare(input);
			}
		}
	}
	for (uint64_t context : {0ULL, 4096ULL, 0xffffc000ULL}) {
		auto input = base; input.contextGgtt = context; Compare(input);
	}
	for (uint64_t ring : {0ULL, 4096ULL, 0xfffff000ULL}) {
		auto input = base; input.ringGgtt = ring; Compare(input);
	}
	// Adjacent intervals legal in either direction; one-page overlap is not.
	for (bool before : {false, true}) {
		auto input = base;
		input.ringGgtt = before ? input.contextGgtt - input.ringBytes : input.contextGgtt + 16384;
		Compare(input);
		input.ringGgtt += before ? 4096 : -4096; Reject(input);
	}
	for (unsigned kind = 0; kind < 23; kind++) {
		auto input = base;
		switch (kind) {
			case 0: input.identity = 0x46d08086; break;
			case 1: input.identity = 0x46d28086; break;
			case 2: input.revision = 1; break;
			case 3: input.revision = 256; break;
			case 4: input.contextGgtt++; break;
			case 5: input.contextGgtt = 0xffffd000; break;
			case 6: input.contextGgtt = UINT64_MAX; break;
			case 7: input.ringGgtt++; break;
			case 8: input.ringGgtt = 1ULL << 32; break;
			case 9: input.ringGgtt = UINT64_MAX; break;
			case 10: input.ringBytes = 0; break;
			case 11: input.ringBytes = 2048; break;
			case 12: input.ringBytes = 12288; break;
			case 13: input.ringBytes = 1U << 22; break;
			case 14: input.ringBytes = UINT32_MAX; break;
			case 15: input.pml4Dma++; break;
			case 16: input.pml4Dma = 1ULL << 39; break;
			case 17: input.pml4Dma = UINT64_MAX; break;
			case 18: input.tail = 0; break;
			case 19: input.tail = 4; break;
			case 20: input.tail = 4096 - 56; break;
			case 21: input.tail = 4096; break;
			case 22: input.tail = UINT32_MAX; break;
		}
		Reject(input);
	}
	alignas(64) uint32_t buffer[4096];
	memset(buffer, 0x5a, sizeof(buffer));
	for (size_t size : {size_t(0), size_t(16383), size_t(16385), SIZE_MAX}) {
		assert(!Image::Build(base, buffer, size));
		for (auto word : buffer) assert(word == 0x5a5a5a5a);
		cases++;
	}
	assert(!Image::Build(base, nullptr, Image::kBytes)); cases++;
	assert(!Image::Build(base, reinterpret_cast<uint32_t*>(UINTPTR_MAX - 3), Image::kBytes)); cases++;
	for (unsigned offset = 1; offset < 4; offset++) {
		assert(!Image::Build(base, reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(buffer) + offset), Image::kBytes));
		for (auto word : buffer) assert(word == 0x5a5a5a5a);
		cases++;
	}
	// Deterministic varied address/root/tail fields, full16KiB comparison each.
	uint64_t random = 0x123456789abcdefULL;
	for (unsigned i = 0; i < 512; i++) {
		random = random * 6364136223846793005ULL + 1;
		auto input = base;
		input.contextGgtt = (random & 0xffff) << 12;
		input.ringGgtt = 0x80000000ULL + (((random >> 16) & 0xffff) << 12);
		input.pml4Dma = ((random >> 12) & ((1ULL << 27) - 1)) << 12;
		input.tail = 8 * (1 + ((random >> 40) % 504));
		Compare(input);
	}
	printf("%u initial context-image cases passed; full16KiB comparisons against extracted pinned Linux routines, CPU-only.\n", cases);
}
