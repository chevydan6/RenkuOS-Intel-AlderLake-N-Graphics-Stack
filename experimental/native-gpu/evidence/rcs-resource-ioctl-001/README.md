# Native resource ioctl on physical N100, 2026-11-14

The owner now optionally allocates and binds 16 exclusive 4 KiB pages at GPU
VA `0x410000` before its first RCS submission. A native, versioned resource
ioctl creates logical buffers in this prebound pool, returns owner-selected
addresses, and transfers up to 4 KiB per call. The owner validates file-local
session and generation-tagged handles, bounds, and GPU retirement before
reusing a range. Creation zeroes backing before exposing it to another file.
The bridge contains no GPU-resident backing. No RenkuOS base-kernel code or
Linux DRM/GEM interface changed.

The [native build](native-build.log) passed and rebuilt binaries match the
installed owner (`db1d192075cf2b33ed1ea298714589db657224d200eb38abdaed818f399baac0`)
and bridge (`e764f6e9f3bd8e9b04a11526dd7eb9657c93be6b15a2c1429a9d70bb481ace3e`).
The prior verified owner and bridge remain at
`/boot/home/native-rcs-resource-012/known-good-owner` and `known-good-bridge`.
Host ASan/UBSan passed arena and ioctl validation tests; their native CPU
versions passed in the complete build. [Source hashes](source.sha256) identify
this checkpoint.

The first hardware attempt returned `B_DEV_NOT_READY` from resource session
open even though the [owner gate log](arena-initialization-failure.log) showed
the pool bound, RCS retired, lifecycle ready, valid ownership, and direct DMA.
The arena's nonzero constructor state was unavailable in the kernel add-on.
Explicit owner initialization fixed the same failing create probe without
changing the bridge or ioctl contract.

After reboot and explicit RCS bootstrap, the resource client passed create,
zeroed readback, a 32-byte transfer crossing a page boundary, bounds rejection,
second-file handle rejection, destroy/stale-handle rejection, and zeroing on
reuse. It passed [64 successive sessions](resource-64.log). These first
regressions used owner `1a141f57fafc191b96c6bea2a72b0e2d1ee811e6db2d0174dc3aed4a721bccd4`
and bridge `3469bf183a27030f595df36d8f822a58d7a52d53716c74dff637eb2dabf506eb`.
The fixed
[v1](v1-64/metadata.json), user-vertex [v2](v2-64/metadata.json), and separate
vertex-page [v3](v3-64/metadata.json) controls each passed 64 physical draws
with their established pixel hashes and consecutive completion sequences.
[Post-resource draws and final probe](post-resource-regression.log) passed;
the v3 draw completed at sequence 194 with 777/777 checked pixels and hash
`28616d2d`.

The bridge initially leaked a created resource if copying the result to a
read-only user buffer failed. The failed-copyout test exhausted the pool on
that bridge. The final bridge rolls back the resource, and the same test
[passed after reboot](final-copyout-regression.log), followed by passing v1,
v2, and v3 draws and a final probe. The physical test lock was released; the
Govee outlet was not used.

These new logical buffers have GPU addresses and CPU read/write access, but
no user-created buffer has yet been consumed by RCS. The pool is fixed at
64 KiB and boot retained; dynamic page-table updates, general command buffers,
fences, shader compilation, and the Vulkan API remain incomplete.
