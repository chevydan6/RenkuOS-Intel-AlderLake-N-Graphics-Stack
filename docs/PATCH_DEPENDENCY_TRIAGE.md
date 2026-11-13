# Patch dependency and migration triage

Snapshot: `gpu/renkuos-n100` at `30dfdb3`. **Filename classification is triage only**, not verified application order or a semantic dependency graph. See [complete immutable inventory](PATCH_INVENTORY.md).

## Patch families (118 files)
| Filename-based family | Count | Migration treatment |
| --- | ---: | --- |
| GART / memory | 20 | Keep in original numbered series until owner, allocator and mapping dependencies are reviewed |
| Display / HDMI | 31 | Preserve known-working physical HDMI baseline; no changes without physical display regression |
| ACPI / DMA | 13 | Preserve kernel lifecycle and descriptor integration order |
| GT / execution | 8 | Keep distinct from unproven live submission |
| Interrupt / lifecycle | 2 | Require teardown/retirement review |
| Other / integration | 44 | Inspect diff paths individually; filename-only classifier cannot resolve scope |

## Direct diff-header sampling (patches 0001–0012)
- 0001, 0003–0006 touch `src/add-ons/kernel/busses/agp_gart/intel_gart.cpp` (some also `intel_gtt.h`).
- 0002 and 0007 touch accelerant hooks/engine; 0008 touches graphics driver power.
- 0009–0011 touch generic `src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp`.
- 0012 touches graphics driver `intel_extreme.cpp`.

The first 12 were inspected for diff target paths; the other 106 have **not** been individually inspected for touched paths or dependency edges in this pass. Do not assume file numbering means all patches were applied.

## Relocation policy
- Keep `patches/`, `workstreams/gpu-next/`, `tests/evidence/`, and `research/` paths unchanged until all scripts, hashes, references and production application state are audited.
- Stage 1 doc moves: `PERFORMANCE.md`, `ARCHITECTURE.md`, `BLIT_RESEARCH.md` copied byte-for-byte into `docs/`; root compatibility pointers preserve old links.
- Moved documents were selected because their Markdown bodies contain no relative Markdown link destinations. This is **not** a guarantee that scripts or external users never read the original files.
- Verify source and build references with `git grep` on a real checkout, then run native VM and physical tests on the development machine before changing code paths.

## Explicitly unverified
No native compilation, physical N100 test, full 118-patch diff inspection, or automated cross-file reference scan has been run by this GitHub-only migration.