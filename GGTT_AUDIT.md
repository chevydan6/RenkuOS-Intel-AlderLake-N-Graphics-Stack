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

Hardware facts were checked against Intel's
[MGGC register definition](https://edc.intel.com/content/www/us/en/design/publications/12th-generation-core-processor-datasheet-volume-2-of-2/pci-mirror-of-gmch-graphics-control-mggc0-0-2-0-pci-offset-50/)
and the MIT-licensed Linux v7.0
[GGTT implementation](https://github.com/torvalds/linux/blob/v7.0/drivers/gpu/drm/i915/gt/intel_ggtt.c)
and [ADL-P device information](https://github.com/torvalds/linux/blob/v7.0/drivers/gpu/drm/i915/i915_pci.c).
New helpers implement documented encodings; no Linux driver source was imported.

## Remaining ownership and coherency work

- Read the target's actual bridge GGC and GPU mirror, compare table sizes and
  resource limits. The initial sudo attempt was denied before reading GGC; the
  user then supplied authentication. The query awaits the shared hardware window.
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
- Continue modern GT/display initialization, power, interrupt, command validation
  and reset work before physical patched-driver installation.

Patches 1–4 passed native compilation and software tests in RenkuOS. Patches 5–6
have not yet completed the native build; the shared power-test reservation blocked
the attempted window. None has been loaded on the physical Intel GPU.
