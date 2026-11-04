<!-- SPDX-License-Identifier: MIT -->
# Alder Lake GGTT audit

The source pin is `4799bb5a188c4bb6bbef967287640cc21a061a0d` plus the numbered
private patches. All findings here are distinct from physical hardware validation.

| Finding | Change or remaining work |
| --- | --- |
| Gen8+ GGTT is at BAR offset 8 MiB | Patch 3 checks the entire table range and overflow. |
| Inherited stores use 32-bit entries and legacy high-address packing | Patch 4 uses 64-bit PTEs, eight-byte stride and the ADL-P/N 39-bit DMA limit for Alder Lake only. Other families retain their existing path. |
| Table bytes were divided by 4096 to count PTEs | Patch 4 divides by eight for Alder Lake and checks aperture coverage. Scratch writes stop at the CPU-visible aperture, preserving mappings beyond it. |
| Bind/unbind always reported success | Patch 4 propagates invalid alignment/address/range errors. Reserved PTEs and adjacent entries are protected by tests. |
| Default physical mappings permit an existing memory-type override | Patch 5 requests explicit uncached MMIO/GGTT mappings for Alder Lake. |
| Fixed-window GGTT still reaches legacy i830 control programming | Patch 5 skips those writes for Alder Lake. It does not initialize GT engines. |
| GMS decoding can silently retain the default size; the last stolen page is subtracted | Patch 6 rejects reserved/zero display configurations and preserves the complete documented stolen extent. |
| Accelerant initialization programs a legacy ring without an Alder Lake backend | Patch 7 skips ring setup/teardown and withholds unsupported engine/overlay hooks. Kernel initialization remains a separate audit. |
| Alder Lake reaches the i965 clock-gating fallback | Patch 8 returns unsupported before those writes. It does not implement modern clock gating or GPU power management. |

Hardware facts were checked against Intel's
[MGGC register definition](https://edc.intel.com/content/www/us/en/design/publications/12th-generation-core-processor-datasheet-volume-2-of-2/pci-mirror-of-gmch-graphics-control-mggc0-0-2-0-pci-offset-50/)
and the MIT-licensed Linux v7.0
[GGTT implementation](https://github.com/torvalds/linux/blob/v7.0/drivers/gpu/drm/i915/gt/intel_ggtt.c)
and [ADL-P device information](https://github.com/torvalds/linux/blob/v7.0/drivers/gpu/drm/i915/i915_pci.c).
New helpers implement documented encodings; no Linux driver source was imported.

## Remaining ownership and coherency work

- The target bridge and GPU GGC mirror both read `0xfec1`: 60 MiB DSM and
  8 MiB GGTT. BAR0 is `0xbf000000`; the calculated table window is `0xbf800000`.
  These were read-only Fedora queries; no RenkuOS hardware mapping was exercised.
- The generic GART manager's `reserved_size` means already-backed memory that
  clients may allocate. It is **not** a firmware exclusion mechanism. Separate
  scanout, WOPCM and protected stolen subregions before allowing general use.
- Inspect reserved physical regions and firmware PTEs. GMS decoding alone does
  not prove that the first GMS bytes of GGTT are safe for client allocation.
- Linux selects GGTT cache/invalidation behavior according to mapping type and
  GuC use. An uncached Gen12 PTE store and a CPU `wbinvd` do not establish a
  complete submission/context invalidation protocol. GuC is not initialized here.
- The legacy non-Alder-Lake PGTBL_CTL path uses a `uint32*` with byte constants;
  its offset arithmetic needs a separately scoped regression fix. Alder Lake
  bypasses that path in patch 5.
- Generic GART unbinding uses the original allocation length after adapting a
  range across stolen memory. Audit partial failures and cleanup before relying
  on it for arbitrary client mappings. Do not free DMA-visible memory early.
- `BindMemory()` sets `BIND_APERTURE` only after every page succeeds. On an
  intermediate failure, `allocate_memory()` calls `DeleteMemory()`, whose
  unbind rejects the unset flag before `_Free()` releases pages. Earlier
  successful PTEs can therefore survive allocation cleanup. Add fault-injected
  rollback tests and an ownership-preserving failure path before enabling DMA;
  returning a page-store error alone is insufficient.
- Continue modern GT/display initialization, power, interrupt, command validation
  and reset work before physical patched-driver installation.

## Kernel and accelerant follow-up

At the pinned source, `intel_extreme_init()` still allocates the 64 KiB primary
ring and overlay page even when patch 7 leaves the accelerant engines disabled.
`intel_en_gating()` falls through to the i965 writes at `0x6204` and `0x7408`
for Alder Lake in the original source. Patch 8 withholds that fallback;
generation-specific gating is still unimplemented. The power agent owns CPU and
ACPI changes; this GPU change requires no shared power-interface modification.

The status-page allocation passes the pointer's value instead of its address.
After shared-info zeroing, generic GART rejects the null output with
`B_BAD_VALUE`, leaving the page unallocated. This is not evidence of a null write:
`allocate_memory()` validates the output pointer first. Decide whether this
obsolete resource belongs in the Alder Lake path before repairing allocation.

Accelerant `disable_fences()` uses the inherited Gen6 fence programming for all
generations above six. Display and interrupt paths also remain active. The
legacy-ring guard therefore does not make physical initialization safe by itself.

The existing `B_APERTURE_NON_RESERVED` allocation flag starts placement after
the already-backed prefix; `B_APERTURE_NEED_PHYSICAL` does likewise. Reuse this
facility for future kernel-owned scratch buffers after verifying firmware
mapping ownership. It is not a substitute for preserving arbitrary boot
scanout/PTE ranges or for correcting partial-bind cleanup. Avoid creating an
unnecessary second aperture allocator before these existing paths are assessed.

Patches 1–7 passed native compilation and software tests in RenkuOS after the
shared power-test window cleared. None has been loaded on the physical Intel GPU.
