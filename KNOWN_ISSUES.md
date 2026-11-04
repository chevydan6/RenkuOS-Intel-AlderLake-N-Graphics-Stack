# Known issues

- No physical Haiku Intel attachment/display/commands have been tested.
- Patches 1–8 compile and pass software tests in the RenkuOS nightly guest.
  No physical driver execution exists.
- N100 461c:46d1 GART pair is missing upstream; the local patch is not hardware validation.
- Legacy 2D hooks are advertised despite unsupported LAKE/SOC0 engines; local
  safeguard suppresses hooks. Patch 7 also skips Alder Lake legacy ring setup
  and teardown, reports zero engines and withholds engine/overlay hooks.
- Kernel initialization still allocates obsolete ring/overlay resources and
  rejects its incorrectly passed status-page output pointer. Patch 8 withholds
  the i965 clock-gating fallback on Alder Lake; its five native checks pass. Modern display,
  interrupt and fence programming remain unvalidated; patch 7 does not fix them.
- Legacy ring-space timeout can continue writing; synchronization token support
  is incomplete. No Gen12 submission should use that path without redesign.
- Patch 3 corrects the GGTT offset; patch 4 implements bounded 64-bit PTE stores
  for Alder Lake. Other generations retain the inherited entry-writing path.
- GART's already-backed-memory region is not an exclusion list. Firmware stolen
  subregion ownership and DMA teardown remain open. Patches 9/10 fix unbind range
  and owned-backing cleanup in host fault-injected tests; native validation is
  pending. Borrowed-buffer pinning and concurrent aperture lifetime remain open.
- Cache/TLB and GuC/context coherency must be established for actual execution;
  a CPU cache flush does not prove GPU completion or context invalidation.
- Current local app_server drawing does not consume legacy 2D hooks.
- No per-process GPU VM, modern validated command submission, complete hang
  recovery, Iris backend or ANV backend has been implemented in this project.
- The target's active GPU cannot be reassigned without explicit authorization
  and verified recovery. IOMMU isolation is necessary but insufficient.
- Native RenkuOS builds use the matching hrev60072+518 SDK; whole-image build and
  hardware ABI validation remain outstanding.
- Haiku upstream rejects AI-assisted contributions. Private development is
  authorized; RenkuOS is now the primary target after source/policy inspection.
