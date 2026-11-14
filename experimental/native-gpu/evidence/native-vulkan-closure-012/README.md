# Native Vulkan direct-source closure checkpoint 012

Date: 2026-11-15 (Asia/Tokyo). Branch: `codex/i915-native-adaptation`.

The VM compiled the current default `NativeVulkan.cpp`, `api_probe.cpp`,
`triangle_probe.cpp`, `fence_probe.cpp`, and `command_reset_probe.cpp` with
`-MMD`. Their five GCC depfiles were passed to `vulkan/measure-closure.py`.
The script first rejected the new depfile set; after its accepted probe list
was updated, it hashed all direct source and header dependencies on the VM.
The resulting [manifest](manifest.json) has 25 unique files and 1,553,904
canonical source bytes:

| Category | Files | Bytes |
|---|---:|---:|
| Pinned Khronos Vulkan headers | 15 | 1,459,702 |
| Native Vulkan implementation | 1 | 48,662 |
| Native GPU ABI | 2 | 4,748 |
| Intel draw fixture and license | 2 | 8,610 |
| Five probe inputs | 5 | 32,182 |

This is a direct compile-input inventory for the default Vulkan library and
probes. It excludes the native GPU owner, Intel compiler, SPIRV-Tools, build
generators, and other runtime dependencies. It is therefore **not** a
complete-stack size or proof of the 3–5 MB target. The model tokenizer was
not available; `model_tokenizer` and `model_tokens` are null in the manifest.
No byte-to-token estimate was substituted.

The 15 headers were extracted from the verified Mesa 26.3.0 archive with
`vulkan/prepare-headers.py`. VM bundle:
`/boot/home/test-work/compiler-pipeline-restored-008`. The depfiles were
generated with `g++ -std=c++17 -O2 -Wall -Wextra -Werror -MMD -c` against
that bundle. The previous
[native Intel compiler closure](../native-haiku-compiler-vm-003/README.md)
is a separate measurement and must not be added without removing shared-file
duplicates and accounting for SPIRV-Tools.
