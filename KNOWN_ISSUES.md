# Known issues

- No physical Haiku Intel attachment/display/commands have been tested.
- No RenkuOS guest runtime test has been performed by this GPU project yet;
  the RenkuOS component build used the preserved Haiku beta6 guest.
- N100 461c:46d1 GART pair is missing upstream; the local patch is not hardware validation.
- Legacy 2D hooks are advertised despite unsupported LAKE/SOC0 engines; local
  safeguard suppresses hooks only. Existing ring setup and other MMIO remain.
- Legacy ring-space timeout can continue writing; synchronization token support
  is incomplete. No Gen12 submission should use that path without redesign.
- Inherited GGTT mapping uses a 2 MiB BAR offset; patch 3 selects 8 MiB for
  Gen8+ and checks its range. Native compilation of patch 3 is pending.
- `set_gtt_entry()` still uses `write32` and legacy address packing, incompatible
  with Gen8+ 64-bit PTEs. Entry counting/indexing, allocation limits, invalidation
  and teardown need a separate coordinated fix. The offset patch is insufficient.
- Current local app_server drawing does not consume legacy 2D hooks.
- No per-process GPU VM, modern validated command submission, complete hang
  recovery, Iris backend or ANV backend has been implemented in this project.
- The target's active GPU cannot be reassigned without explicit authorization
  and verified recovery. IOMMU isolation is necessary but insufficient.
- Native build uses beta6 installed kernel headers; whole-image build and
  hardware ABI validation remain outstanding.
- Haiku upstream rejects AI-assisted contributions. Private development is
  authorized; RenkuOS is now the primary target after source/policy inspection.
