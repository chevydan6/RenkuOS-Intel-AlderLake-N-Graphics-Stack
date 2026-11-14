# Offline N100 PPGTT bindings, 2026-11-14

`IntelN100PpgttTree::BindPages` can bind externally owned 4 KiB pages only
inside unused slots of the existing 2 MiB GPU VA window. It validates every
page, address, PAT index, duplicate DMA, and target PTE before writing any
entry. `SealBindings` rejects subsequent changes. The caller still owns the
external physical backing and must retain it until GPU retirement; this
checkpoint adds no post-publication PTE mutation or TLB invalidation.

The test first failed with stubbed binding methods. The completed test passed
under host ASan/UBSan and in the isolated native N100 build; see
[host-sanitizer.log](host-sanitizer.log) and [native-smoke.log](native-smoke.log).
The complete native candidate build ended with
`NATIVE_GPU_EXECUTION_BUILD_PASSED`. [Owner hashes](owner-hashes.txt) show the
new candidate differs from the unchanged installed v2 owner. Source hashes
are in [source.sha256](source.sha256).

This is a CPU/offline page-table result. No new PTE was published to hardware,
no owner-backed BO exists yet, and no Vulkan command was submitted.
