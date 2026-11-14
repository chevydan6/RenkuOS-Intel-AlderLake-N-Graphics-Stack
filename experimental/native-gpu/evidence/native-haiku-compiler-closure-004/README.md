# Native Haiku Mesa SPIR-V compiler closure

Date: host 2026-11-15 JST. The [native compiler proof](../native-haiku-compiler-vm-003/README.md)
linked Mesa 26.3.0's Vulkan SPIR-V translator, NIR, and Intel N100 backend
inside the disposable Haiku VM. This measurement uses its **actual native
link map and Ninja include dependencies**, via
[`measure-haiku-closure.py`](../../render/compiler/mesa-26.3.0/measure-haiku-closure.py).
The relinked compiler ran on the validated Vulkan 1.0 shader pair and produced
all five artifacts byte-identical to the previously verified native build.

| Input class | Files | Full file bytes |
|---|---:|---:|
| Mesa source and headers | 533 | 14,025,279 |
| Project compiler probe and identity header | 2 | 8,716 |
| Generated build files | 49 | 62,392,205 |
| External Haiku headers and compiler inputs | 293 | 4,990,336 |
| Total distinct included files | 877 | 81,416,536 |

The linker selected 352 object files. The relinked executable was 14,616,304
bytes, and its only ELF `NEEDED` libraries were `libstdc++.so.6`,
`libgcc_s.so.1`, and `libroot.so`. It did not link a Linux DRM library.
The native source closure already exceeds the experimental 3–5 MB target
before adding the Vulkan API implementation or native kernel driver. This
is a measured starting point, not a proof that a smaller specialized compiler
is impossible.

The machine-readable [`summary.json`](summary.json), linked
[`objects.txt`](objects.txt), included [`files.txt`](files.txt), and compressed
raw [`link.map.xz`](link.map.xz) preserve the measurement. The uncompressed
link map was 2,305,085 bytes, SHA-256
`ad20280f145f985757eda07b73911f36bf06db7599071969ad3520e217f6a95e`.
Its compressed storage size and the executable size are separate from source
bytes and AI tokens. No development-model tokenizer was available for this
experiment, so no token-count claim is made.

The inventory counts complete source and header files, including inactive
code within them. It excludes build-only generator sources, package metadata,
tests, and the OS implementation. It is the selected native compiler link and
include closure, not the complete graphics stack or a minimal implementation.
