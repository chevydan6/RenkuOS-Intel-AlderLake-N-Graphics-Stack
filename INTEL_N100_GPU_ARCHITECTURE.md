<!-- SPDX-License-Identifier: MIT -->
# Intel N100 GPU architecture: independent research

2026-11-07. Terra research for Astra; no production implementation or physical execution claimed.
Target 8086:46d1, reported revision 0. Linux source pin: `028ef9c96e96197026887c0f092424679298aae8`.
All statements marked **Verified** mean verified in the cited source, not on RenkuOS hardware.
**Proposed** means an engineering recommendation; **Open** requires further evidence.
Sources and hashes: [manifest](research/terra-n100/evidence/linux-references.json).

## Immediate conclusion

**Proposed:** first execute one fixed kernel-owned MI store on BCS0 using a logical
ring context and execlists. Put the store directly in the ring for the earliest
proof; a separate PPGTT batch is a subsequent milestone. The logical context,
valid address-space root, GT initialization, cache visibility and recovery still
have to exist. An old legacy BLT ring-tail write does not provide this path.
See [packet research](GPU_COMMAND_SUBMISSION_RESEARCH.md) and
[implementation order](GPU_FIRST_EXECUTION_PLAN.md).

## Device identity and address widths

**Verified:** `INTEL_ADLN_IDS` selects `adl_p_info`; that table supplies Gen12
features, BCS0 alongside render/media engines, 48-bit PPGTT and a 39-bit DMA mask.
The inherited Gen11 features enable ELSQ and declare noncoherent GGTT.
The logical context address and ring start use GGTT offsets. GPU virtual address
width is distinct from the physical address width accepted by PTEs.

[static const struct intel_device_info adl_p_info](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/i915_pci.c#L681); [INTEL_ADLN_IDS](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/i915_pci.c#L862); [#define GEN11_FEATURES](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/i915_pci.c#L603).

**Open:** live engine availability/fuses, exact stepping-to-workaround mapping,
DMA reachability/IOMMU policy and active firmware ownership must be measured on
the target. A static Linux engine mask is not live engine discovery. No PCI
probe was performed during this review.

## Bring-up requirements and source map

| Area | Verified Linux behavior | RenkuOS implication |
| --- | --- | --- |
| GT initialization | `intel_gt_init` creates scratch/VM and engines, initializes uc and resumes GT | Implement a staged lifecycle and unwind every completed stage |
| Hardware initialization | `intel_gt_init_hw` holds forcewake, applies/verifies GT workarounds, configures PPGTT, initializes selected uc and global MOCS | Display register mapping is insufficient GT bring-up |
| Engines | `intel_engines_init_mmio` prepares inventory; `intel_engines_init` selects GuC or execlists | Discover BCS0, establish power references and one submission owner |
| Logical context | `reg_offsets` selects `gen12_xcs_offsets`; `lrc_init_state`, `lrc_update_regs` initialize context/ring | Use Gen12 XCS relative register layout; do not reuse old render-context offsets |
| GPU VM | `gen8_ppgtt_create` selects `gen12_pte_encode` | Construct owned page tables and scratch entries; use Gen12 PAT and system-memory encoding |
| Cache policy | `get_mocs_settings` selects `gen12_mocs_table` for ADL-P; `intel_mocs_init` initializes global policy | MOCS source gap in Astra's earlier design is now resolved; PAT and actual visibility still need review |
| Translation | `intel_gt_invalidate_tlb_full` chooses GuC/MMIO invalidation, serialized with reset and power | Posting PTE writes plus CPU cache flush does not establish engine TLB invalidation |
| Retirement | Gen12 XCS breadcrumb uses stalling flush before sequence store; execlists processes Gen12 CSB | Separate payload observed, request completed, context inactive and memory releasable |
| Reset | Engine prepare/reset/finish and GT reset manage submission and state | Stop new work, capture errors, bound reset, reinitialize, reject stale completions |

Precise sources:

- [int intel_gt_init(struct](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_gt.c#L685); [int intel_gt_init_hw(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_gt.c#L171).
- [int intel_engines_init_mmio(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_engine_cs.c#L953); [int intel_engines_init(struct](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_engine_cs.c#L1489).
- [static const u8 gen12_xcs_offsets[]](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_lrc.c#L227); [void lrc_init_state(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_lrc.c#L1016); [u32 lrc_update_regs(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_lrc.c#L1537).
- [struct i915_ppgtt *gen8_ppgtt_create(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/gen8_ppgtt.c#L1002); [static u64 gen12_pte_encode(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/gen8_ppgtt.c#L61).
- [static unsigned int get_mocs_settings(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_mocs.c#L454); [void intel_mocs_init(struct](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_mocs.c#L666).
- [void intel_gt_invalidate_tlb_full(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_tlb.c#L121).
- [static int gen8_engine_reset_prepare(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_reset.c#L560); [static int __gen11_reset_engines(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_reset.c#L519); [void intel_gt_reset(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_reset.c#L1201).

## Firmware decision

**Verified:** `uc_expand_default_options` enables GuC submission on the relevant
modern path by policy. `intel_engines_init` retains execlists selection when
GuC submission is not used. Consequently Linux source supports researching an
execlists path without a GuC scheduler; it does not prove RenkuOS can omit every
firmware/power requirement on this N100.

**Verified:** ADL-N firmware selection is specially redirected to ADL-S because
ADL-N lacks HWConfig. The ADL-S GuC table selects the TGL family, with preferred
70.12.1 version tuple at this pin. A major-version filename and required/selected
version are distinct; do not infer `adlp` firmware from the shared device-info table.

[static void uc_expand_default_options(](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/uc/intel_uc.c#L26); [p = INTEL_ALDERLAKE_S;](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/uc/intel_uc_fw.c#L300); [fw_def(ALDERLAKE_S,  0, guc_maj](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/uc/intel_uc_fw.c#L96).

**Proposed:** keep GuC disabled explicitly for the isolated execlists experiment,
with logged initialization state and no competing scheduler. If GuC becomes
necessary, budget firmware verification/upload/authentication, WOPCM, CT transport,
ADS/context registration, scheduler lifecycle and reset recovery as a separate
implementation. HuC media authentication is not a prerequisite established by
this MI-store experiment. DMC/display firmware is a separate display concern.

## Existing RenkuOS boundary

The current GART code manages CPU aperture/GGTT mappings. It has no demonstrated
modern execution service or GPU request ownership. Its CPU `wbinvd` and PTE
readback cannot prove GT idle, engine TLB invalidation or request retirement.
The existing below-4-GiB page restriction is conservative relative to Linux's
39-bit DMA mask; it is not evidence of a 48-bit physical addressing capability.

**Open before execution:** forcewake domain/ack definitions, complete PAT setup,
exact context-size cross-check against PRM, Gen12 CSB initialization/consumption, applicable
ADL stepping workarounds, reset-only hardware behavior and unload retention.
Linux definitions are implementation evidence; this review has not independently
cross-checked every field against the Intel Alder Lake PRM. Do not call the
register sequence hardware-ready until that review is recorded.

## Provenance and scope

Reference files are research artifacts, with their upstream notices preserved.
Do not copy Linux implementations into RenkuOS without checking each file's
license and the project's provenance requirements. No OpenGL, Vulkan, HDMI audio
or rendering pipeline investigation was undertaken.
