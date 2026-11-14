# Resource policy CPU checkpoint, 2026-11-14

`NativeResource.h` introduces a CPU-only, file-local resource handle and GPU
VA reservation policy. It rounds sizes to 4 KiB, rejects invalid or
overlapping bindings, rejects stale handles, and seals new bindings after GPU
publication. Destroyed published handles become inaccessible but remain
retained until the owner proves the VM epoch retired. The table does not
allocate physical backing, edit PPGTT entries, submit commands, or expose a
new ioctl.

The test first failed on the missing implementation and then on its stubbed
`Create`. The completed test passed under host ASan/UBSan and native N100 CPU
execution; see [host-sanitizer.log](host-sanitizer.log) and
[native-smoke.log](native-smoke.log). The full isolated native build script
also ended with `NATIVE_GPU_EXECUTION_BUILD_PASSED`. Source hashes are in
[source.sha256](source.sha256). The installed owner hash remains the known
working v2 build in [installed-owner.sha256](installed-owner.sha256).

This checkpoint establishes resource policy only. It does not prove GPU BO
mapping, Vulkan memory allocation, post-publication VM binding, or a general
RCS submission ABI. Those are the next implementation tasks.
