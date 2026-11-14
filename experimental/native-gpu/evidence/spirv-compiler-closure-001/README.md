# Pinned Mesa SPIR-V compiler closure and DRM link removal

On the host, `spirv/measure-closure.py` rebuilt the Mesa 26.3.0
SPIR-V → NIR → Intel N100 shader compiler harness from the existing Meson
graph. It used the compiler's actual static archive resolution, Ninja's
recorded include dependencies, and the linker map. This selected **354 linked
objects**, **318 compiled source files**, and **1,018 distinct included files**.
The measured file inventory is in [summary.json](summary.json).

| File origin | Files | Bytes |
|---|---:|---:|
| Mesa canonical tree | 535 | 14,150,712 |
| Mesa build outputs | 49 | 62,392,205 |
| Host external headers | 433 | 6,074,136 |
| RenkuOS harness source | 1 | 8,616 |

These are complete file sizes, including inactive code, broad generated
tables, and transitive headers. They are **not** a minimum N100 compiler source
set or a model-token count. The 49.5 MB generated NIR algebraic C file alone
dominates the build-output total. Generated files and external headers are
listed separately from canonical Mesa source. The inventory excludes source
of build-time generators that did not appear in compiler dependencies.

The unmodified host link retained four dynamic `libdrm` symbols through
`intel_device_info.c` and `intel_kmd.c`. The harness selects the N100 profile
by PCI ID and does not call their file-descriptor discovery functions. Linking
the same static objects with `--gc-sections` and omitting `libdrm.so` produced
a working executable with **no `libdrm` dynamic dependency or undefined DRM
symbols** (`readelf -d` and `nm -D`). Compiling the validated Vulkan 1.0
vertex and fragment SPIR-V modules with that executable produced byte-for-byte
identical VS and FS machine code to the hardware-tested artifacts:

| Artifact | SHA-256 |
|---|---|
| no-DRM host compiler executable | `f929b5d1cb0ad688477f2bb2bb8b30ef1a9e84ee2d8689c1a667f17b7be19fa1` |
| VS machine code | `cf08637755fa472911ca133f49e64693b5d1f460f2963dd5ec5e44dbb3cdab58` |
| FS machine code | `b9f73778f45cfbd379a7f7d960fc047b5e75505231cc1356c718ff46b1b3e18b` |

The source closure still **includes** Mesa DRM headers and Linux-specific
code. The successful link demonstrates only that Linux DRM is unnecessary for
the tested compiler execution path. A Haiku-native compiler build still needs
OS facility adaptation and a suitable Mesa build configuration. No N100
hardware or installed driver was changed during this measurement.

Reproduce with the bootstrapped host toolchain:

```sh
source /home/dan/projects/native-rcs-compiler-001/final-tools/env.sh
python3 experimental/native-gpu/render/compiler/mesa-26.3.0/spirv/measure-closure.py \
  /home/dan/projects/native-rcs-compiler-263-final \
  /home/dan/projects/new-compiler-closure
```

The output directory must not exist. `summary.json`, `objects.txt`,
`files.txt`, and `link.map` remain there for audit. The manifest paths are
specific to the local build workspace and are deliberately not copied here.
