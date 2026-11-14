# Public buffer triangle on physical N100, 2026-11-14

The native resource ioctl created a GPU buffer at owner-selected VA
`0x410000`, copied three validated position vectors into it, and passed its
file-local handle to RCS draw version 4. The owner resolved the handle under
its device lock, validated the buffer contents and bounds, and patched only
the vertex-buffer address in its known-good batch. The fixed private vertex
page was zeroed for this draw, so the passing image requires the GPU to read
the public buffer. No user GPU address, packet, shader, or register offset was
accepted. No RenkuOS base-kernel code or Linux DRM/GEM interface changed.

The [native build](native-build.log) passed. [Installed hashes](installed-hashes.txt)
match rebuilt owner `a071c7f708c4cb4095efba86abd4caf6eb10fb0a418256fd5b0551dd7c48f8cc`
and bridge `1aa5c71cb7bc29ade698c93e82b74fd5834c612773ff388a4f065b1ba57d4907`.
The previous verified owner and bridge remain in the isolated N100 bundle as
`known-good-v3-resource-owner` and `known-good-v3-resource-bridge`.
Host ASan/UBSan passed the vertex packet test; its native CPU version passed
in the complete build. [Source hashes](source.sha256) identify the change.

After reboot, read-only probe and explicit RCS bootstrap passed. Three
[invalid requests](final-negative-and-probe.log)—an unwritten buffer, a
second-file forged handle, and a destroyed handle—were rejected with zero
sequence and output. The first valid public-buffer draw returned 777/777
checked pixels, image hash `28616d2d`, matching completion sequence and no
GPU fault.

| Physical regression | Result | Final sequence |
| --- | --- | ---: |
| [64 public-buffer v4 draws](v4-64/metadata.json) | Four expected images and gap-free completion | 66 |
| [64 fixed v1 draws](v1-64/metadata.json) | Existing baseline preserved | 130 |
| [64 user-vertex v2 draws](v2-64/metadata.json) | Existing baseline preserved | 194 |
| [64 separate-page v3 draws](v3-64/metadata.json) | Existing baseline preserved | 258 |
| [Four final v4 draws](v4-after-controls/metadata.json) | Public buffer still passes | 262 |

Every runner retained JSONL records and raw client output. Resource
create/read/write/destroy, failed-copyout rollback, invalid v4 requests, and
the final read-only probe then passed. The physical test lock was released;
the Govee recovery outlet was not used.

This is a genuine GPU draw from a userspace-created native buffer. It is still
a bounded RCS workload with owner-selected shaders and packets, a fixed 64 KiB
prebound pool, synchronous ioctl completion, and no Vulkan queue, SPIR-V shader
path, general command-buffer validation, fence API, or native presentation.
