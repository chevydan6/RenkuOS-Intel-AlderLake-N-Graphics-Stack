// SPDX-License-Identifier: MIT
// Copyright © 2003-2018, 2019 Intel Corporation
// Native integration Copyright (c) 2026 chevydan6
#ifndef NATIVE_GPU_COPY_REQUEST_IMAGE_H
#define NATIVE_GPU_COPY_REQUEST_IMAGE_H
#include "RequestImage.h"
#include <LinearCopy.h>
namespace NativeGpu {
// Fixed private PPGTT source/destination pages, neither aliases scanout. The
// owner builds these two mappings and initializes source/destination itself.
// Packet/BLIT_CCTL ordering follows pinned v6.6 gem/selftests/
// i915_gem_client_blt.c::prepare_blit(), retaining Intel's MIT hardware logic.
inline bool BuildCopyRequestImage(const ImageInput& in,
	const ImageBuffers& cpu, ImageOutput& out, bool initializeStatus = true)
{
	if (!BuildRequestImage(in,cpu,out,initializeStatus)) return false;
	// Insert 3-dword BLIT_CCTL LRI + 10-dword copy + alignment NOOP between
	// nonce stores and the existing ordered flush/completion/preemption tail.
	memmove(cpu.ring+22,cpu.ring+8,18*sizeof(uint32_t));
	cpu.ring[8]=0x11000001;
	cpu.ring[9]=0x22204;
	cpu.ring[10]=0x606; // src/dst MOCS index3 (uncached), encoded index<<1.
	Gen12Experiment::LinearSurface src={0x200000,4096,4096};
	Gen12Experiment::LinearSurface dst={0x201000,4096,4096};
	if (!Gen12Experiment::PackLinearCopy32(dst,src,1024,1,cpu.ring+11,10)) return false;
	cpu.ring[21]=0;
	out.tail+=56;
	out.workaroundTail+=56;
	IntelN100ContextImage::Input context={in.identity,in.revision,in.ggtt[3],
		in.ggtt[0],in.rootPhysical,4096,out.tail};
	return IntelN100ContextImage::Build(context,cpu.context,16384);
}
}
#endif
