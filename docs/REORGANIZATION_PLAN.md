# Non-destructive repository reorganization plan

This branch intentionally begins with documentation and inventories; **no existing code or evidence has been moved yet**.

## Target logical layout (proposal, not an accomplished migration)
- `drivers/native/`: existing native driver patch/application sources, after build audit.
- `drivers/linux-i915/`: selected experimental port sources, only after technical review.
- `compatibility/linuxkpi/`: genuinely shared Linux-to-RenkuOS interfaces.
- `tests/unit/`, `tests/native-vm/`, `tests/physical/`: executable tests, separated from evidence.
- `evidence/verified/` and `evidence/historical/`: immutable, hash-linked artifacts.
- `docs/`: authoritative status, architecture, roadmap, decisions.
- `archive/`: superseded design and agent narratives, never silently discarded.

## Ordered migration gates
1. Inventory all 118 numbered patches: ordering, target paths, touched symbols, whether applied in production, and associated evidence. Distinguish `workstreams/gpu-next` patches.
2. Build file-reference graph across Markdown, Makefiles, Jamfiles, scripts, manifests, CI and hash manifests. Record all relative references and generated paths.
3. Compare `experiment/i915-linuxkpi` against native baseline at API/ABI boundaries, not merely file counts. Preserve physical HDMI build 057 and current RenkuOS source baseline.
4. Extract one canonical architecture map: PCI discovery -> retained memory owner/GART -> GGTT/PPGTT -> power/forcewake -> BCS reset -> context/ring -> submission/completion -> future Mesa.
5. Move only self-contained documentation first, rewriting all links; validate Markdown references and CI. Preserve Git rename traceability.
6. Relocate source/test/evidence in small batches, with mechanical path rewrites, native VM compilation and tests performed on the development machine.
7. Compare before/after evidence hashes and test results. Require Terra independent review of any ownership, DMA, locking or completion changes.
8. Open PR for review; never automatically merge. Do not delete historical branches until separately approved.

## Agent roles
Sol: implement small batches and run builds. Terra: independent technical validation and evidence audit. Astra: review architecture and investigate genuine stalls, not continuous planning. Watchdog: deterministic monitoring and resume/recovery without routine model calls.

## First technical questions
- Does the LinuxKPI port materially shorten the path to first GPU command, or introduce larger Linux kernel-subsystem dependencies?
- Which native memory-owner, forcewake, reset and HDMI components are reusable with the port?
- What exact missing functions/interfaces block the real ring request and retirement?
- Which patches and tests are actually required by the currently deployed N100 build?
