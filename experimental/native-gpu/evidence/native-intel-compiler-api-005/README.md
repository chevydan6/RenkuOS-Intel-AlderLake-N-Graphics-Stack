# Native in-process Intel SPIR-V compiler API

Date: host 2026-11-15 JST. Mesa source: pinned 26.3.0 archive, SHA-256
`91d729aa68bb89e4ec857841300bbe922bcb3e52b1f97566ca83930d3b28343a`.
This ran in the disposable Haiku VM with no N100 PCI passthrough. The physical
N100, owner driver, and base kernel were unchanged.

[`IntelSpirvRuntime.h`](../../render/compiler/mesa-26.3.0/IntelSpirvRuntime.h)
defines a versioned C ABI. Its implementation uses Mesa's SPIR-V translator,
NIR, Intel device profile, and Intel compiler in process, taking caller-owned
SPIR-V words and code buffers. The output contains GPU program bytes and
selected stage metadata. The ABI is Intel-specific and does not impose Intel
fields on a future hardware-neutral Vulkan interface. The source adapter
[`prepare-haiku.py`](../../render/compiler/mesa-26.3.0/prepare-haiku.py)
adds `librenku_intel_compiler.so` to the native Mesa build.

The shared library linked natively, 14,611,840 bytes, SHA-256
`d9bf2d3b3c8af87749c734dc4706dfbb718b770bbde6a7b67d69bdbb58290d53`.
Its ELF `NEEDED` libraries were `libstdc++.so.6`, `libgcc_s.so.1`, and
`libroot.so`; no Linux DRM library was linked. The library exports
`renku_intel_compile_vulkan_pair`.

The [C smoke program](../../render/compiler/mesa-26.3.0/runtime_smoke.c)
linked against the shared library. It rejected an invalid SPIR-V header and
an undersized output buffer. With the validated Vulkan 1.0 shader pair, it
returned 128-byte VS and 208-byte FS programs byte-identical to the
hardware-tested compiler artifacts. With the fragment constant changed from
`0.25` to `0.5`, the VS stayed identical and the FS changed to the expected
SHA-256 `ea9fadfaeaa943979f55e30be1b48dc0732b041061699819d34f2bca2e7ef2aa`.
The repeatable results are in [`smoke.txt`](smoke.txt), and the build failure
and successful retry logs are retained in [`logs`](logs/). The VM was saved as
checkpoint `native-compiler-api-001` after verification.

With the pinned Mesa tree prepared as in the
[VM compiler proof](../native-haiku-compiler-vm-003/README.md), build the
shared target and link the smoke program against it:

```sh
ninja -C build-compiler -j 1 src/intel/compiler/brw/librenku_intel_compiler.so
gcc -std=c11 -Wall -Wextra -Werror -I. runtime_smoke.c \
  -Lbuild-compiler/src/intel/compiler/brw -lrenku_intel_compiler -o runtime_smoke
LIBRARY_PATH=build-compiler/src/intel/compiler/brw:/boot/home/config/lib:/boot/system/lib \
  ./runtime_smoke triangle.vert.spv triangle.frag.spv vs.bin fs.bin
```

This API compiles the current bounded N100 shader profile. It does not yet
validate complete SPIR-V semantics, handle descriptors or specialization,
support arbitrary Vulkan pipeline state, or connect to `libvulkan.so`.
The owner still accepts only its previously proven fragment program. Runtime
hardware testing and pipeline integration await a verified physical N100.
