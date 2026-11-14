// SPDX-License-Identifier: MIT
// Copyright © 2014 Intel Corporation
// Native integration Copyright (c) 2026 chevydan6
#ifndef NATIVE_GPU_REQUEST_IMAGE_H
#define NATIVE_GPU_REQUEST_IMAGE_H
#include <IntelN100ContextImage.h>
#include <IntelN100ContextStatus.h>
#include <IntelN100CsbReader.h>
#include <IntelN100NonceStores.h>

namespace NativeGpu {
// GGTT slots: ring, engine HWSP, result, context. The native owner supplies
// exclusive mapped backing; these integers never establish that ownership.
struct ImageInput {
	uint32_t identity, revision;
	uint64_t ggtt[4], rootPhysical;
	uint32_t nonce, sequence, tag;
};
struct ImageBuffers { uint32_t *ring, *status, *result, *context; };
struct ImageOutput { uint64_t descriptor; uint32_t tail, workaroundTail; };
static constexpr uint32_t kPoison = 0x5a5a5a5a;

// Native integration of Linux v6.6 gen12_emit_fini_breadcrumb_xcs/tail and
// gen8_emit_wa_tail. Intel's MI command ordering and Gen12 preemption point are
// retained. Polling completion replaces MI_USER_INTERRUPT with MI_NOOP because
// the native display driver's IRQ handler does not own this request.
// CPU preparation only: callers still establish DMA admission, cache policy,
// engine initialization and lifetime before any hardware publication.
inline bool BuildRequestImage(const ImageInput& in, const ImageBuffers& cpu, ImageOutput& out)
{
	if (!cpu.ring || !cpu.status || !cpu.result || !cpu.context
		|| !in.sequence || in.sequence == kPoison
		|| in.nonce == kPoison || ~in.nonce == kPoison) return false;
	const uint64_t sizes[] = {4096, 4096, 4096, 16384};
	for (unsigned i = 0; i < 4; i++) {
		if ((in.ggtt[i] & 4095) || in.ggtt[i] > (1ULL << 32) - sizes[i]) return false;
		for (unsigned j = 0; j < i; j++)
			if (in.ggtt[i] < in.ggtt[j] + sizes[j] && in.ggtt[j] < in.ggtt[i] + sizes[i]) return false;
	}
	ImageOutput image = {};
	image.tail = 24 * 4;
	image.workaroundTail = 26 * 4;
	if (!intel_n100_bcs_descriptor(in.identity, uint8_t(in.revision), in.ggtt[3], in.tag, &image.descriptor))
		return false;
	IntelN100ContextImage::Input context = {in.identity, in.revision, in.ggtt[3],
		in.ggtt[0], in.rootPhysical, 4096, image.tail};
	if (!IntelN100ContextImage::Build(context, cpu.context, 16384)
		|| !intel_n100_initialize_hwsp(cpu.status, 4096)) return false;
	memset(cpu.ring, 0, 4096);
	memset(cpu.result, 0x5a, 4096);
	cpu.status[0x40] = kPoison; // Separate from CSB and the zero preempt semaphore.
	if (!intel_n100_ggtt_nonce_stores(in.ggtt[2], in.nonce, cpu.ring, 1024)) return false;
	const uint32_t completion[] = {
		0x13000002, 0, 0, 0, // Stalling MI_FLUSH_DW before the post-sync store.
		0x13004002, uint32_t(in.ggtt[1] + 0x100) | 4, 0, in.sequence,
		0, 0x04000001, // Polling completion; MI_ARB_ON_OFF | MI_ARB_ENABLE.
		0x02800000, // MI_ARB_CHECK: trigger idle-to-active before the wait.
		0x0e40c003, 0, uint32_t(in.ggtt[1] + 0xc8), 0, 0,
		0x02800000, 0 // WaIdleLiteRestore padding; beyond the normal tail.
	};
	memcpy(cpu.ring + 8, completion, sizeof(completion));
	out = image;
	return true;
}
}
#endif
