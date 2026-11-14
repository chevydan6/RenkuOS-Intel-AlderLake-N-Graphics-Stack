# Separate owner-backed RCS vertex page, physical N100, 2026-11-14

The RenkuOS `intel_memory_owner` kernel add-on now optionally allocates one
exclusive, direct-DMA 4 KiB page and binds it at GPU VA `0x408000` before the
first RCS publication. The PPGTT binding is sealed immediately before the
first publish. Draw version 3 copies validated user vertices into that page
and changes only the vertex-buffer address in the existing owner-built batch.
The shaders, target, and other packets remain the proven v2 workload. An
allocation or bind failure disables v3 without changing v1/v2 admission.
No RenkuOS base-kernel code or Linux DRM/GEM interface changed.

The installed candidate owner SHA-256 is
`4f176be703101468b540788f4817ef1cbb102bcfe2876291eae45249cdc744e4`.
The previous v2 owner remains at
`/boot/home/native-rcs-resource-007/known-good-v2-owner` with SHA-256
`1e188b4ef7b46125278833c92af99a5de924075ea6e32da42d17e31e84b42dc9`.
The candidate rebooted on PCI `8086:46d1`, revision 0; read-only probe and
explicit RCS bootstrap both passed. The complete [native build](native-build.log)
passed after the final client change, and its rebuilt owner hash matched the
installed binary. Host ASan/UBSan also passed the PPGTT binding and vertex
packet tests. [Source hashes](source.sha256) identify the change.

| Physical sequence | Result | Final sequence |
| --- | --- | ---: |
| [64 fixed v1 draws](v1-64/metadata.json) | Four baseline hashes, 654/654 pixels each | 66 |
| [64 v2 user-vertex draws](v2-64/metadata.json) | Four baseline hashes, checked pixels matched | 130 |
| [64 v3 resource-vertex draws](v3-64/metadata.json) | Same four hashes and pixel counts as v2 | 194 |
| [Four final v1 draws](v1-after-v3/metadata.json) | Baseline preserved | 198 |

Each runner saved per-draw raw output and JSONL records beside its metadata.
The three malformed v3 inputs (NaN coordinate, non-unit `w`, zero-area
triangle) were [rejected](invalid-and-final-probe.log) with no sequence or
output. A valid v3 draw immediately afterward completed at sequence 199 with
777/777 checked pixels and hash `28616d2d`; the final read-only probe passed.
The physical test lock was released. The Govee recovery outlet was not used.

This proves a separate owner-allocated, GPU-addressed vertex resource is read
by RCS on the actual N100 and that completion/readback still works. The page
is fixed and boot-retained; there are no general BO create/map/destroy ioctls,
dynamic VM binding, arbitrary validated Vulkan command submission, fences, or
Vulkan application yet. Those remain the next implementation steps.
