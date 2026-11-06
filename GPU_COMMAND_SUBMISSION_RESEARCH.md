<!-- SPDX-License-Identifier: MIT -->
# Minimum Intel N100 command submission

2026-11-07. Independent Terra research, Linux pin `028ef9c96e96197026887c0f092424679298aae8`.
**Verified** denotes source evidence. **Proposed** denotes the RenkuOS design.
No physical execution is claimed.

## Smallest useful workload

**Verified:** Linux's LRC selftest emits a four-dword `MI_STORE_DWORD_IMM_GEN4 |
MI_USE_GGTT` directly into a request ring. This is a smaller payload than a
second-level batch, BLT copy, shader or render operation.
[LRC selftest direct-ring store](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/selftest_lrc.c#L97).

**Proposed:** use one BCS0 logical context, one outstanding kernel request and a
private GGTT result page. Emit a nonce and its complement to two distinct words;
then emit ordered completion to a separate status location. This demonstrates
command-streamer execution, not EU shader execution, rendering or acceleration.

Payload packet, source-derived candidate (not a complete executable ring):

| Dword | Value | Meaning |
| --- | --- | --- |
| 0 | `0x10400002` | `(0x20 << 23) | (1 << 22) | 2`: MI store, GGTT selection, four-dword form |
| 1 | `result_ggtt_offset + word_offset` | Dedicated aligned GGTT destination, never CPU pointer or DMA address |
| 2 | `0` | Upper address for this below-4-GiB GGTT mapping |
| 3 | `nonce` | GPU writes immediate value |

[#define MI_STORE_DWORD_IMM_GEN4](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_gpu_commands.h#L136); [#define   MI_USE_GGTT](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_gpu_commands.h#L139).

Keep payload words naturally aligned and within their page. Allocate completion
at page offset 0 (or another reviewed slot) so it is 8-byte aligned and address
bit 5 is clear, as required by the reference breadcrumb emitter.
A fixed diagnostic may use privileged kernel construction; this is not an API
for untrusted userspace batches or arbitrary GGTT stores.

## Structures that must exist before submission

| Structure | Address consumed by GPU | Minimum responsibility |
| --- | --- | --- |
| GT scratch | According to each scratch command | Dedicated owned page, initialized, not display memory |
| Engine HWSP/CSB | GGTT offset via HWS register | Correct Gen12 CSB layout, initialized pointer, error/completion evidence |
| Logical context including per-process HWSP and register image | GGTT LRCA | Required full engine context size/alignment, Gen12 XCS register image |
| Ring | GGTT start in context | Valid page-multiple size, aligned head/tail, bounds and reserved tail |
| PPGTT root and scratch hierarchy | Physical/DMA root in PDP0 context fields | Valid four-level address space; never uninitialized even for GGTT payload |
| Result and completion | Dedicated GGTT offsets | Pinned private pages, poison/guards, CPU cache and GPU visibility contract |
| Request record | CPU only | References to all above, sequence/reset generation, deadline and retirement state |

A direct GGTT store removes the separate batch buffer and useful PPGTT payload
mapping from this first experiment. It does **not** establish that a null PPGTT
root or incomplete context is permitted. Whether a reduced scratch hierarchy is
sufficient requires explicit register/PRM review; implement the valid supported
hierarchy first. Page tables contain DMA addresses, not GGTT offsets.

[static void init_ppgtt_regs(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_lrc.c#L889); [static u32 lrc_descriptor(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_lrc.c#L1521); [u32 lrc_update_regs(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_lrc.c#L1537).

## Submission mechanism: ELSQ on this family

**Verified:** ADL-N inherits `has_logical_ring_elsq`. Linux configures SQ contents
at `engine_base + 0x510` and control at `engine_base + 0x550`.
`write_desc` writes low then high dword for each ELSQ slot; unused slots are
zeroed, ports are written in descending order, then control receives
`EL_CTRL_LOAD` (bit 0). The queue is not cleared automatically.
The older high/low writes to a single ELSP port are a different path.

[static void write_desc(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_execlists_submission.c#L723); [static void execlists_submit_ports(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_execlists_submission.c#L905); [#define RING_EXECLIST_SQ_CONTENTS](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_engine_regs.h#L232).

**Verified:** `execlists_update_context` updates the context's saved ring tail
and issues `wmb()` before publication. The descriptor contains GGTT LRCA and
addressing/privilege/valid flags, with Gen11+ engine/class/context-ID format in
the upper dword. A Gen8 context-ID packing is incorrect for ADL-N.

[static u64 execlists_update_context(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_execlists_submission.c#L672); [static u32 lrc_descriptor(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_lrc.c#L1521).

`enable_execlists` requires forcewake; it disables legacy mode, clears STOP_RING,
programs HWS PGA and configures error reporting. Global and engine MOCS setup
must precede execution. Context image ring start/head/tail/control replace the
assumption that writing the inherited legacy RING_TAIL submits modern work.

[static void enable_execlists(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_execlists_submission.c#L2928); [static int execlists_resume(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_execlists_submission.c#L2952).

**Open:** cross-check source-derived BCS0 base, forcewake/reset definitions and all
masked-write semantics against exact target documentation; the offsets above
are relative definitions, not a complete MMIO programming recipe.

## Completion and visibility

**Verified:** Gen12 XCS completion emits a stalling `MI_FLUSH_DW` before its
sequence store. `gen8_emit_ggtt_write` emits a four-dword flush with post-sync
store, GGTT selection in the address dword and alignment/bit-5 checks. The Gen12
tail emits user interrupt/arbitration and may include preemption workarounds.
Do not transplant the sequence store alone or copy render PIPE_CONTROL logic.

[u32 *gen12_emit_fini_breadcrumb_xcs(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/gen8_engine_cs.c#L807); [gen8_emit_ggtt_write(u32 *cs](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/gen8_engine_cs.h#L129); [gen12_emit_fini_breadcrumb_tail(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/gen8_engine_cs.c#L786).

**Proposed:** initially poll coherent status with bounded deadlines while still
configuring/handling the required engine error and context events. IRQ delivery
is an optional wakeup mechanism, not the sole completion proof. Check payload,
expected sequence/reset generation, Gen12 CSB context transition and idle state.
A lost interrupt with a valid completed request differs from a GPU timeout.
A visible nonce does not imply the context has stopped accessing its backing.

## Optional next step: batch-buffer execution

After direct-ring proof, map an immutable batch into the private PPGTT. A store
without `MI_USE_GGTT` targets PPGTT (review privilege/addressing fields first),
followed by `MI_BATCH_BUFFER_END` (`0x05000000`) and padding. The ring's Gen8+
batch-start form contains header, lower GPU VA, upper GPU VA; Linux uses bit 8
for the non-secure/PPGTT dispatch path, plus arbitration handling.
Terminate the second-level batch, not the logical ring, with BBE.

[int gen8_emit_bb_start(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/gen8_engine_cs.c#L574); [#define MI_BATCH_BUFFER_START_GEN8](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_gpu_commands.h#L188); [#define MI_BATCH_BUFFER_END](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_gpu_commands.h#L62).

This second step verifies PPGTT address translation as well as execution.
Packet encodings above are source-verified candidates; final emitted ring,
context, page-table entries, barriers and applicable workarounds require Astra's
design plus independent review before hardware publication.


## Additional source-verified bring-up details

- BCS0 selects BLT_RING_BASE = 0x22000. Therefore SQ contents = 0x22510,
  control = 0x22550, HWS PGA = 0x22080 and ring mode = 0x2229c at this pin.
  These are GTTMMADR MMIO offsets, not GGTT memory addresses. See [#define BLT_RING_BASE](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/i915_reg.h#L265).
- Copy context image size is two 4-KiB pages (including ppHWSP). Linux's Gen12
  allocator then adds two separate workaround pages, giving 16 KiB before
  optional debug redzone. Do not confuse the base context image with total
  state allocation. See [#define GEN8_LR_CONTEXT_OTHER_SIZE](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_engine_cs.c#L50) and [__lrc_alloc_state(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_lrc.c#L1074).
- The Gen12 forcewake range table assigns 0x22000–0x23fff to GT, rather than
  the old MEDIA-domain BLT rule. GT request/ACK definitions are 0xa188/0x130044;
  render request/ACK are 0xa278/0x0d84. Initialization and fallback ACK handling
  must follow [static int intel_uncore_fw_domains_init(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/intel_uncore.c#L2158),
  the Gen12 range table and masked-write protocol; raw request addresses alone
  are insufficient initialization. Keep GT power/runtime ownership distinct
  from forcewake references.
- Gen12 BCS TLB invalidation register is 0xcee4; BCS0 uses unmasked bit 0 and
  waits for it to clear. The source additionally applies Wa_2207587034 via
  GEN12_OA_TLB_INV_CR when engines are awake. Invalidate/reset are serialized
  and forcewake held. See [#define GEN12_BLT_TLB_INV_CR](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_gt_regs.h#L1051) and [static void mmio_invalidate_full(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_tlb.c#L49).
- GT workaround dispatch for this family reaches gen12_gt_workarounds_init,
  including Wa_14011060649 and Wa_14011059788 and special handling for firmware
  locks on MISCCPCTL. Engine/context workarounds have separate dispatch;
  evaluating only this GT function is insufficient. See [gen12_gt_workarounds_init(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_workarounds.c#L1507).

These close source lookup gaps; target PRM/stepping and hardware review remain.
