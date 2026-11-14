<!-- SPDX-License-Identifier: MIT -->
# Native Intel GPU request preparation

Active direction: native adaptation retaining Intel hardware logic. See
[the current handoff](../../NATIVE_GPU_HANDOFF.md).

RequestImage.h reuses the existing native Gen12 context/CSB/nonce helpers and
adapts Linux v6.6's XCS completion sequence for a polling native request.
prepare_module.cpp connects that image to the existing GGTT memory owner and
the validated native page allocation bridge. Native compilation/export linking
and one physical preparation invocation now pass; GPU execution remains pending.
See [native-prepare-001](evidence/native-prepare-001/README.md).

The focused host image smoke and one native preparation call pass. No engine publication occurs here;
the mapping/v1 contract remains memory-only. Actual DMA admission, execution
ownership, engine programming, completion/retirement and the accelerated sample
remain to implement. Preserve their distinction when reporting progress.

Hardware sequence reference: Linux v6.6 commit
ffc253263a1375a65fa6c9f62a893e9767fbebfa,
drivers/gpu/drm/i915/gt/gen8_engine_cs.c and gen8_engine_cs.h,
gen12_emit_fini_breadcrumb_xcs/tail, gen8_emit_wa_tail and
__gen8_emit_flush_dw/gen8_emit_ggtt_write. The polling adaptation uses a NOOP
instead of requesting an interrupt that the native display handler does not own.

Ported-from: Linux/v6.6/drivers/gpu/drm/i915/gt/gen8_engine_cs.c
Ported-from-license: MIT

Ported-from: Linux/v6.6/drivers/gpu/drm/i915/gt/gen8_engine_cs.h
Ported-from-license: MIT

The reused native helpers retain their own original licenses and provenance in
workstreams/gpu-next and research/n100-submission. New native integration is MIT.
