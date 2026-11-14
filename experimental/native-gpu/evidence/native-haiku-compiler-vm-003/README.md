# Native Haiku Vulkan shader compiler proof

Date: host 2026-11-15 JST. This ran in the disposable `renkuos-tests`
Haiku VM without N100 PCI passthrough. The Mesa source was the pinned 26.3.0
archive, SHA-256
`91d729aa68bb89e4ec857841300bbe922bcb3e52b1f97566ca83930d3b28343a`.
No physical N100 GPU command or kernel change occurred in this experiment.

## Result

Mesa configured natively for Haiku with Gallium, Vulkan drivers, OpenGL,
EGL, GBM, GLX, LLVM, shader cache, zlib, and zstd disabled. The two
compiler-only targets use Mesa's own NIR, SPIR-V translator, Intel device
description, and Intel shader backend. The direct-NIR target linked after
670 Ninja steps. The SPIR-V target subsequently linked and accepted the
validated Vulkan 1.0 vertex and fragment modules:

| Artifact | SHA-256 | Verification |
|---|---|---|
| VS machine code, 128 bytes | `cf08637755fa472911ca133f49e64693b5d1f460f2963dd5ec5e44dbb3cdab58` | Byte-identical to hardware-tested host artifact |
| FS machine code, 208 bytes | `b9f73778f45cfbd379a7f7d960fc047b5e75505231cc1356c718ff46b1b3e18b` | Byte-identical to hardware-tested host artifact |
| SPIR-V metadata | `fded68a2d1586a12b3ae3b2fa8af69557cac30574862e3c33992f14abb880d54` | Byte-identical to host SPIR-V proof |

Both native targets ran twice with byte-identical output. All five direct-NIR
artifacts match the host direct-NIR proof; all five SPIR-V artifacts match
the host SPIR-V proof. The native SPIR-V executable's SHA-256 was
`aa915795dadd5f2fc5d28cb45177e380ed7b1b85a9c8ac58ed1cc84e794d4d1b`.
Its Haiku ELF `NEEDED` entries were `libstdc++.so.6`, `libgcc_s.so.1`, and
`libroot.so`; no `libdrm` runtime library was linked. Its 14 MB binary size
is not a source-code or token count.

The [artifact checker](../../render/compiler/mesa-26.3.0/check_artifacts.py)
passes for both copied native outputs. For the SPIR-V output, pass
`--harness-source` with `spirv/compile_spirv.c`; its metadata records that
source hash rather than the direct-NIR harness hash.

## Reproduce

The source adapter is
[`prepare-haiku.py`](../../render/compiler/mesa-26.3.0/prepare-haiku.py).
Run it on a fresh extracted Mesa 26.3.0 source tree. Keep it beside
`native_rcs_shaders.c` and `spirv/compile_spirv.c`, as in the repository.
It is idempotent and refuses to overwrite changed probes. On the Haiku guest,
set `LIBRARY_PATH` to include the home and system library directories,
put the verified Meson/Ninja wrappers first in `PATH`, and include the
unpacked Mako/MarkupSafe dependencies in `PYTHONPATH`:

```sh
export LIBRARY_PATH=/boot/home/config/lib:/boot/system/lib:/boot/system/non-packaged/lib
export PATH=/boot/home/test-work/haiku-tool-wrappers-001:$PATH
export PYTHONPATH=/boot/home/test-work/haiku-python-builddeps-001/unpacked
export TMPDIR=/boot/home/test-work/tmp
python3.10 prepare-haiku.py mesa-26.3.0
meson setup build-compiler mesa-26.3.0 -Dbuildtype=release \
  -Dgallium-drivers= -Dvulkan-drivers= -Dplatforms= -Dglx=disabled \
  -Degl=disabled -Dgbm=disabled -Dopengl=false -Dgles1=disabled \
  -Dgles2=disabled -Dllvm=disabled -Dintel-elk=false -Dbuild-tests=false \
  -Dexpat=disabled -Dzstd=disabled -Dzlib=disabled -Dshader-cache=disabled
ninja -C build-compiler -j 1 src/intel/compiler/brw/native-spirv-shaders
build-compiler/src/intel/compiler/brw/native-spirv-shaders \
  artifacts-spirv triangle.vert.spv triangle.frag.spv
```

The SPIR-V inputs have SHA-256
`2600423ddbd1575456ed163da45593361c41811e45ba413b53afb9fcdd72315a`
(vertex) and
`d1ad98503d1d100de336747219852879f65cf769e72fc63d0a96f3760a0efe60`
(fragment). Both were assembled and validated for Vulkan 1.0 in the
[host proof](../../render/compiler/mesa-26.3.0/spirv/README.md).

The first VM build stopped when GCC wrote a large generated NIR assembly
temporary under the VM's nearly full 650 MB boot volume. Setting `TMPDIR`
to the 16 GB workspace let it finish. A later link reported ENOSPC once,
then succeeded unchanged on retry; the workspace had over 15 GB free.
These failures and the successful build are preserved in [`logs`](logs/).
The VM was checkpointed as `native-compiler-built-001` after the first
successful native SPIR-V build. The final adapter-built target was also
compiled and checked against the same artifact bytes.

This proves native compilation of one validated Vulkan shader pair.
It does not yet provide arbitrary SPIR-V support in `libvulkan.so`, runtime
pipeline integration, Vulkan conformance, or a native hardware test of the
compiler-built executable. The physical N100 was still offline after its
separate BFS panic.
