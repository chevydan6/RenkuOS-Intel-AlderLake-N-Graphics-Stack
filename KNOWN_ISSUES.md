# Known issues

- No physical Haiku Intel attachment/display/commands have been tested.
- N100 461c:46d1 GART pair is missing upstream; the local patch is not hardware validation.
- Legacy 2D hooks are advertised despite unsupported LAKE/SOC0 engines; local
  safeguard suppresses hooks only. Existing ring setup and other MMIO remain.
- Legacy ring-space timeout can continue writing; synchronization token support
  is incomplete. No Gen12 submission should use that path without redesign.
- Current local app_server drawing does not consume legacy 2D hooks.
- No per-process GPU VM, modern validated command submission, complete hang
  recovery, Iris backend or ANV backend has been implemented in this project.
- The target's active GPU cannot be reassigned without explicit authorization
  and verified recovery. IOMMU isolation is necessary but insufficient.
- Native build uses beta6 installed kernel headers; whole-image build and
  hardware ABI validation remain outstanding.
- Haiku upstream rejects AI-assisted contributions. Private development is
  authorized; RenkuOS migration investigation now takes priority.
