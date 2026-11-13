# Repository audit — 2026-11-13

Scope: `chevydan6/haiku-intel-gpu`, default branch `gpu/renkuos-n100` at `30dfdb32957215b4c59da3ac763ba14ab0c482d0`. This is a structural audit, not a line-by-line source review or new physical validation.

## Reproducible inventory
Git tree `fbb1ca1c2ab2772720d0de5ccdcb74bfdcb00dc9`: 1,206 blobs, 1,351 tree entries including directories; tree not truncated. Directories by blob count: tests 743 (568 under tests/evidence), research 165, patches 118, workstreams 46, results 25, tools 22, installation 19, coordination 15, physical 14, diagnostics 8, experiments 6, experimental 2. Root has 19 Markdown documents plus metadata/license files. Counts are snapshot-specific.

## Branch relationships at audit time
- `gpu/renkuos-n100`: authoritative default baseline.
- `gpu/alderlake-n`: 172 commits behind default, no unique ahead commits.
- `gpu/native-memory`: 34 behind, no unique ahead commits.
- `gpu/hdmi-pipe-fix`: diverged, 5 ahead / 148 behind; inspect its 11 changed files before archival.
- `experiment/i915-linuxkpi`: 6 ahead / 0 behind; 207 changed files; independent experimental implementation. **Do not merge blindly.**
- `rondoflow/n100-results-20261112`: 47 ahead / 0 behind; 46 changed files, primarily cycle reports, not driver source integration.

## Current verified-vs-unverified boundary
README and results report physical N100 detection, 1920x1080 RGB8 HDMI, one bounded physical BCS reset, and physical memory/forcewake diagnostics. They explicitly do **not** demonstrate GPU command execution, accelerated rendering or Vulkan. The LinuxKPI experiment reports native VM and physical CPU kernel-services driver tests (16 x 62 checks each), imported ring/allocator code, and CPU packet construction; it explicitly does **not** demonstrate GPU execution or a full working i915 port.

## Organizational findings
1. Historical and current status intermingle in root README and chronological results. A single current status index is needed.
2. Tests and archived evidence dominate the tree; distinguish test source, immutable evidence, and summaries without deleting evidence.
3. `patches/` (118 patches) and `workstreams/gpu-next/` (additional patches, headers, scripts) need an explicit dependency and application inventory before relocation.
4. Research and agent cycle outputs should not be treated as implementation evidence.
5. The default branch, LinuxKPI experiment, and RondoFlow publication branch contain distinct kinds of work and must not be collapsed without review.
6. Existing source build scripts, relative document links, patch paths and evidence hashes may depend on current locations.

## Non-destructive policy
No bulk move, deletion, rebase, or merge until the reference graph is checked and Sol runs the native build/test suite. Preserve all original branches and evidence; migration must be reversible.
