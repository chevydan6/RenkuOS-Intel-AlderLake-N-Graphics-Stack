<!-- SPDX-License-Identifier: MIT -->
# Intel N100: minimum safe command submission

Status: proposed architecture, 2026-11-07. No execution implementation or physical
acceleration result is claimed. Target: 8086:46d1 revision 0; reject other devices
until separately validated. Preserve physical HDMI baseline 057. This document
addresses the first real execution milestone, not only memory regression work.

## Decision and experiment

Implement one kernel-owned copy-engine context, one private GPU address space,
one outstanding request, and a fixed diagnostic operation. Prefer execlists as
the initial mechanism, conditional on confirming the N100 stepping's context,
workaround and reset requirements. Do not revive the inherited legacy ring.
Do not implement a GuC loader merely to reproduce Linux's default policy.
If execlists cannot satisfy those gates, stop that branch and implement the
required GuC lifecycle explicitly; there is no register-poking fallback.

The first experiment is a GPU-generated nonce-dependent write into a private
result page, followed by an ordered completion breadcrumb and context retirement.
Use a reviewed MI store command on BCS0, not an initial 3D pipeline or display
surface. This proves command execution, not rendering or acceleration speed.
A second experiment copies randomized offscreen data with a validated modern
BLT packet and compares every byte against a CPU reference. Neither experiment
writes the scanout buffer or changes the known-working modeset sequence.

The first operation must be kernel constructed. Userspace supplies only an
experiment selector and nonce, never a batch, GPU address, register, or backing
page. Arbitrary submission, multi-client scheduling and Mesa are later layers.
This avoids needing a general userspace command validator for the first proof;
it does not waive buffer lifetime, page-table or recovery requirements.

## Pinned Linux evidence and limits

Reference pin: Linux `028ef9c96e96197026887c0f092424679298aae8`. Retrieved reference
files and SHA-256 values are recorded in `references.json`. They are research
material, not code linked into RenkuOS. Review MIT notices and preserve attribution
and Ported-from trailers before importing any implementation. One attempted
MOCS reference fetch returned HTTP 429; no MOCS implementation was verified here.

| Area | Verified reference | Consequence for this design |
| --- | --- | --- |
| Device | [i915_pci.c](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/i915_pci.c), adl_p_info and INTEL_ADLN_IDS | ADL-N uses ADL-P device info; listed engine mask includes BCS0, GPU VA size 48 bits and DMA mask 39 bits. Discover live engine availability; GPU VA width is not physical DMA width. |
| Submission choice | [intel_engine_cs.c](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_engine_cs.c), intel_engines_init | Linux selects GuC when in use, else execlists when supported. This establishes a reference path, not proof of this RenkuOS implementation or stepping's readiness. |
| Firmware policy | [intel_uc.c](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/uc/intel_uc.c), uc_expand_default_options | Modern default policy can enable GuC submission. Policy is distinct from a hard dependency for the proposed execlists path. |
| ADL-N firmware exception | [intel_uc_fw.c](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/uc/intel_uc_fw.c), __uc_fw_auto_select | ADL-N is redirected to ADL-S selection because it lacks HWConfig. At this pin the ADL-S GuC table uses the TGL family (70.12.1 preferred version tuple). Do not choose adlp firmware solely from the platform name. |
| Context | [intel_lrc.c](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_lrc.c) and [execlists](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_execlists_submission.c) | Context-image construction and ring ownership must be ported with generation/stepping handling, not inferred from old ring tail writes. |
| GPU VM | [gen8_ppgtt.c](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/gen8_ppgtt.c) | Private PPGTT needs owned page-table backing and scratch policy. Existing GGTT backing alone does not implement it. |
| Completion | [gen8_engine_cs.c](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/gen8_engine_cs.c), gen12_emit_flush_xcs and breadcrumb helpers | Ordering, flushes and completion stores are engine/generation-specific. A changed memory word alone is insufficient retirement evidence. |
| Reset | [intel_reset.c](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_reset.c) | Reset includes preparation and completion/lifetime repair. Writing a reset bit is not recovery. |

Linux GEM, dma_fence, scatterlists, workqueues, runtime-PM, DRM ioctls and IRQ
helpers are not assumed present in RenkuOS. Adapt documented hardware algorithms;
replace OS mechanisms with explicit RenkuOS ownership, locking, semaphores,
kernel workers, physical allocation and interrupt facilities. HuC media services
are outside the proposed MI-store workload. GuC firmware authentication/transport,
WOPCM and scheduler state would be additional work if the GuC route is selected.
DMC/display firmware policy must remain separate from GT submission initialization.
No claim that all firmware can be skipped has been established on hardware.

## Proposed C++ boundaries

These are proposed responsibilities, not existing APIs:

- `IntelGtDevice`: device/stepping gate, engine inventory, GT-specific power and
  forcewake references, validated register adapter, reset serialization.
- `IntelGpuBuffer`: exact size, backing pages, DMA bounds, CPU cache attributes,
  pin references and quarantine state. Never copy a kernel address to userspace.
- `IntelGpuAddressSpace`: page tables, GPU VA allocation, checked permissions,
  mapping/TLB generation and scratch mappings. It cannot free a pinned mapping.
- `IntelCopyContext`: reviewed LRC image, ring, context/status backing, descriptor
  and context-status-buffer processing for one engine.
- `IntelGpuRequest`: owns context/VM/buffer references, immutable batch, sequence,
  deadline, error and retirement status. Request destruction cannot free live DMA.
- Diagnostic device operation: privileged fixed workload request and bounded
  structured result. Keep legacy accelerant ioctls and shared ABI unchanged.

Keep packet planners pure and MMIO behind narrow checked adapters. Do not put
power, allocation, packet emission and polling into one ioctl function. The
per-device submission mutex serializes one request; a worker owns asynchronous
retirement. Never wait for GPU completion holding the GART global lock or the
accelerant display lock. Document GT/GART lock interactions before implementation.

Allocate all ownership records before resources become live. Objects required by
an in-flight request belong to the kernel device, not the requesting file/team.
Closing the diagnostic client cannot cancel DMA by freeing its pages. The device
must have an explicit unload barrier; if unload cannot be vetoed, the execution
service must be a retained kernel module whose lifetime outlasts quarantined DMA.
Do not simulate safety by leaking an untracked pointer or blindly pinning a module
without a supported lifetime contract. This unload contract remains unresolved.

## Request lifecycle and memory contract

States: Prepared -> Published -> InFlight -> Completed -> Retired -> Released.
A pre-publication failure unwinds immediately. A timeout transitions to Recovering;
only proven quiescence permits Retired. Failed reset transitions to Quarantined:
reject subsequent submissions, preserve every DMA-visible allocation and require
controlled recovery. A timeout is never a completion signal.

Use fresh dedicated system pages, not firmware/stolen or display backing. Validate
physical address width for every mapped page, page alignment, virtual-range
arithmetic, aliases and read/write permissions. Bind page tables, context, ring,
batch, status and result backing before publishing the context. Keep them pinned
until context retirement or reset proves access stopped. Freeze immutable batches
before submission; prohibit client mappings that can race their contents.

The private PPGTT maps only required request objects. GGTT mappings for context/
status objects follow the reviewed hardware addressing mode. Document each
command's address space explicitly; never interchange CPU addresses, GGTT offsets,
GPU VAs and DMA addresses. Establish CPU publication ordering, device cache policy,
TLB invalidation and GPU-to-CPU visibility from the pinned sequence plus Intel
register documentation. Existing wbinvd or GGTT arithmetic tests do not prove
that contract. MOCS/PAT and relevant workarounds remain a pre-execution gate.

## Exact remaining gates

| Gate | Current evidence | Required evidence before workload |
| --- | --- | --- |
| Host lifetime | 100 threaded query/unmap rounds; 154 lifecycle checks; recovery fixtures | Extend to allocate/bind/free and competing reopen; failure/close/unload cases with retained objects. |
| Caller contract | Seven manager callbacks now protect lookup lifetime | Review all clients, including sources absent from sparse checkout; establish behavior after failed unmap and forbid reuse of cleanup-only records. |
| Hardware memory | Native089 built on N100; matched components verified; 60 serial allocation/free cycles across two boots and 10 paired-client rounds (20 allocations/frees) passed. See physical-memory-001/002 and physical-concurrent-001 evidence. | Physical failure/close/reopen recovery and retained backing validation remain incomplete. Paired live ranges do not prove simultaneous kernel execution, mapping extent, or GPU access. |
| Ownership | GART retains failed teardown; reopen retries cleanup | Demonstrate device/module lifetime through client death, failed cleanup and unload; resolve partial-bind orphan recovery before execution. |
| Placement | Page-rounding and reserved-prefix guards; host regressions | Audit remaining nonempty-list/alignment arithmetic, aliasing, DMA pinning and invalidation. |
| GT bring-up | No modern execution path | Read-only engine/stepping inventory, bounded forcewake/GT power, reset-domain and interrupt audit; confirm BCS0. |
| VM/context | Reference files inspected only | Implement and test private PPGTT, context-image layout, context descriptor, status handling, cache policy, generation workarounds and publication order. |
| Recovery | GART cleanup recovery is not engine recovery | Implement bounded engine stop/reset, stale completion rejection, reinitialization and lifetime retention. Test reset-only operation before the first batch. |
| Physical recovery | Test USB components backed up; original image preserved. User reset restored a stalled terminal/USB session. NanoKVM console and visible native builds work after reboot. | Preserve backups and bounded observation. User explicitly accepts sandbox crashes and physical reset; verified remote recovery is not an additional authorization prerequisite. The earlier hang cause remains unresolved. |

Until every gate passes, no workload is authorized by this design. Physical
memory validation is a separate phase from launching a batch. The read-only
hardware inventory is the next hardware-facing step; do not hide an execution
probe inside it. MOCS reference retrieval and register-level stepping audit are
specific research gaps, not reasons to expand unrelated infrastructure.

## First physical execution protocol

1. Under canonical local hardware locks, verify recovery media and backups,
   identify 46d1/revision and capture fresh HDMI/SSH baseline. Boot only isolated
   test media with matched components. Record source, patch, binary and firmware
   hashes (or explicit firmware-not-loaded state), engine inventory and config.
2. Run the fixed operation disabled as a negative control: poison result and
   guard pages, provide nonce, follow the same CPU verification path, and require
   failure because no GPU write occurred. No CPU helper may implement the result.
3. Prepare one BCS0 request. Kernel builds a reviewed MI store of nonce-derived
   words to its result mapping, terminates the batch, and emits the reviewed
   ordered flush/completion sequence. Do not specify final dwords until their
   PRM fields, addressing mode and applicable workarounds are independently checked.
4. Publish once. Proposed watchdog: 250 ms request deadline and a separate 1 s
   recovery budget, with shorter bounded register polls. These are conservative
   experiment limits to validate, not documented hardware response guarantees.
5. Require result match plus monotonic engine/context completion evidence and
   context retirement. Capture batch bytes/hash, programmed addresses, sequence,
   timing and engine status. CPU verifier compares full output/guards after
   the established visibility transition. A store without retirement is failure.
6. Repeat 100 fresh nonce trials with varied owned pages. A second reviewer must
   reproduce results from captured artifacts and the verifier. Cross-check the
   disabled-submission control, guards and absence of CPU writes to result data.
7. Test recovery first with injected software completion loss and deadline
   expiry; do not begin with an intentional hardware hang. Require quiescence
   before freeing buffers. On failure quarantine, stop further execution, preserve
   logs and restore via recovery media. Never assume power-cycle capability
   replaces a required driver recovery path.
8. Verify the HDMI desktop and management access remain functional after each
   trial group. Report command execution separately from later BLT correctness
   and throughput; no OpenGL or desktop-acceleration claim follows from MI stores.

## Implementation and review workstreams

Implementation: kernel-owned memory/lifetime contract, GT inventory/power/reset,
VM/context, fixed request, then physical evidence. Keep each milestone independently
reviewable with signed-off commits and machine-assistance disclosure. Do not commit
untested execution code as a working milestone.

Review: maintain a separate checklist/artifact from implementation, checking exact
register sources, DMA lifetime, unload, address-space selection, packet bounds,
negative controls and preserved display. Do not modify reference expectations merely
to obtain green tests. A self-review is not the project's required independent PR
approval. No additional agents were launched for this design.

After verified MI execution, add offscreen BLT copy with exact byte comparisons,
then measured GPU/software baselines including submission/coherence overhead.
Only then choose a desktop hook integration point. Mesa/Iris support needs a real
buffer/context/sync kernel interface and substantial OS adaptation; linking Mesa
or advertising old hooks is not that implementation.

## Reviewed research update, 2026-11-07

Astra accepts Terra T-F1: first payload is two direct-ring MI stores (nonce and
complement), not a separate batch. This supersedes separate-batch wording above
for the first proof only; context, valid PPGTT root/scratch, completion and reset
remain required. See ../../GPU_FIRST_EXECUTION_PLAN.md and
EXECUTION-OWNERSHIP.md (ownership proposal awaiting Terra review).
Terra T-F2 supplies the missing MOCS reference; register/stepping and visibility
review remains open. T-F3 independently verifies the retained-module mechanism,
not automatic backing/aperture retention. Imported reports are source research,
not a hardware-ready programming recipe.
