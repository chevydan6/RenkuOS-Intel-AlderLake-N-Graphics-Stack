// SPDX-License-Identifier: MIT
#include "CopyRequestImage.h"
#include <assert.h>
#include <stdio.h>
int main()
{
	alignas(4096) uint32_t ring[1024], status[1024], result[1024], context[4096];
	NativeGpu::ImageInput in = {0x46d18086, 0, {0x10000,0x20000,0x30000,0x40000},
		0x100000, 0x1234, 1, 1};
	NativeGpu::ImageBuffers cpu = {ring,status,result,context};
	NativeGpu::ImageOutput out;
	assert(NativeGpu::BuildCopyRequestImage(in,cpu,out));
	assert(out.tail==152 && out.workaroundTail==160);
	assert(ring[8]==0x11000001 && ring[9]==0x22204 && ring[10]==0x606);
	assert(ring[11]==0x50800008 && ring[14]==0x00010400);
	assert(ring[15]==0x201000 && ring[19]==0x200000);
	assert(ring[22]==0x13000002 && ring[29]==1);
	assert(result[0]==NativeGpu::kPoison && status[0x40]==NativeGpu::kPoison);
	status[kN100CsbWrite/4]=10;
	status[kN100CsbBuffer/4]=0x12345678;
	in.sequence=2;in.tag=2;
	assert(NativeGpu::BuildCopyRequestImage(in,cpu,out,false));
	assert(status[kN100CsbWrite/4]==10 && status[kN100CsbBuffer/4]==0x12345678);
	assert(ring[29]==2 && status[0x40]==NativeGpu::kPoison);
	puts("Native copy request image: PASS (CPU construction only)");
}
