# Compiler-backed native Vulkan direct-source closure (VM)

The pinned VM bundle `/boot/home/test-work/compiler-pipeline-restored-008`
was rebuilt with the current compiler-backed Vulkan code and `-MMD` dependency
files for the library and five probes. The compiler pipeline, API, fence, and
command-reset probes passed. `libvulkan.so` SHA-256 was
`ae1d74b463b5c699ebe3a8e5a22dfb3d35149a48107a4fc4310840a715579b0b`,
matching the previously tested physical N100 alternate library. The triangle
probe compiled to SHA-256
`da356407cc1df4ea530762bee808d000e68d9c593e44909ba49a0a23b8b3c41d`;
the VM has no N100 PCI passthrough, so no hardware draw was run here.

`deps/` contains the six GCC dependency files. `manifest.json` hashes and
counts their 31 distinct direct source/header inputs: 1,569,087 bytes. The
native Vulkan implementation source itself is 49,473 bytes; 15 Khronos
headers account for 1,459,702 bytes. The remaining files are native ABI,
render headers, the Intel draw fixture, and probe sources.

This is a direct compile-input closure. It excludes the linked Mesa Intel
compiler, SPIRV-Tools implementation, Haiku system libraries, build-only
generators, and kernel owner. It counts complete header files, including
unused declarations, and cannot establish a minimum Vulkan port size or an
AI model-token count. The compiler link/include measurement is recorded in
[`native-runtime-closure-022`](../native-runtime-closure-022/README.md).
