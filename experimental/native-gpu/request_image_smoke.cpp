// SPDX-License-Identifier: MIT
#include "RequestImage.h"
#include <assert.h>
#include <stdio.h>
int main()
{
	alignas(8) uint32_t ring[1024], status[1024], result[1024], context[4096];
	NativeGpu::ImageInput input = {0x46d18086, 0, {0x10000, 0x20000, 0x30000, 0x40000},
		0x100000, 0x915cafe, 1, 1};
	NativeGpu::ImageBuffers buffers = {ring, status, result, context};
	NativeGpu::ImageOutput out = {};
	assert(NativeGpu::BuildRequestImage(input, buffers, out));
	assert(out.tail == 96 && out.workaroundTail == 104);
	assert(ring[0] == 0x10400002 && ring[1] == 0x30000 && ring[3] == input.nonce);
	assert(ring[7] == ~input.nonce && ring[12] == 0x13004002 && ring[13] == 0x20104);
	assert(status[0x32] == 0 && status[0x40] == 0x5a5a5a5a && result[0] == 0x5a5a5a5a);
	assert(context[1024 + 7] == out.tail && context[1024 + 9] == 0x10000);
	assert(out.descriptor == (0x600000000000011dULL | (1ULL << 37) | 0x40000));
	puts("Native request-image smoke: PASS");
}
