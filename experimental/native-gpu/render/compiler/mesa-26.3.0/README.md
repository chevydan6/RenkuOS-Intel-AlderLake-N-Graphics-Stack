# Mesa 26.3.0 offline N100 compiler experiment

This versioned recipe builds the deterministic VS/FS NIR harness against the
pinned Mesa 26.3.0 archive. The resulting shader bytes passed the physical
N100 RCS regression described in `../../../evidence/rcs-mesa263-shader-001/`.
It is still a host compiler experiment, not a native Vulkan driver. The
previous 26.1.8 artifacts remain available as the earlier known-good image.
The `spirv/` proof assembles Vulkan SPIR-V modules and emits byte-identical
machine code for the same VS and FS.

The archive SHA-256 is
`91d729aa68bb89e4ec857841300bbe922bcb3e52b1f97566ca83930d3b28343a`.
`integrate.py` modifies only a fresh extracted tree. It enables Mesa's Intel
and full NIR build graphs without selecting a GL or Vulkan driver, then adds
the narrow executable using Mesa's declared dependencies. Mesa 26.3.0 uses
the unified `brw_compile` parameters. For this shader, its VS metadata reports
URB read length zero and entry size one; ANV passes the compiler-reported read
length to the VS state packet. The regenerated RCS state image passed the
physical N100 regression.

From the repository root, using the already bootstrapped compiler toolchain:

```sh
source /home/dan/projects/native-rcs-compiler-001/final-tools/env.sh
JOBS=8 experimental/native-gpu/render/compiler/mesa-26.3.0/build.sh \
  /home/dan/projects/mesa-26.3-native/mesa-26.3.0.tar.xz \
  /home/dan/projects/new-native-compiler-263-build
python3 experimental/native-gpu/render/compiler/mesa-26.3.0/check_artifacts_test.py -v
```

The output workspace must not exist before building. The recipe compiles and
runs the harness twice, validates both outputs, and compares all five artifacts.
The checked-in `artifacts/` passed its checker and SHA-256 manifest. Four
mutated-artifact tests also passed, including corrupt VUE mapping, aliased FS
dispatch offsets, scratch, relocation, and invalid code cases.

| Output | Mesa 26.3.0 result |
| --- | --- |
| VS machine code | 128 bytes, SHA-256 `cf08637755fa472911ca133f49e64693b5d1f460f2963dd5ec5e44dbb3cdab58` |
| FS machine code | 208 bytes, SHA-256 `b9f73778f45cfbd379a7f7d960fc047b5e75505231cc1356c718ff46b1b3e18b` |
| Both stages | Zero scratch and unresolved relocations |

The host Meson build can still link host libraries such as libdrm, but this
harness does not access a GPU or require Linux kernel graphics services at
runtime. A subsequent [native Haiku compiler proof](../../../evidence/native-haiku-compiler-vm-003/README.md)
built both this direct-NIR harness and a Vulkan SPIR-V harness inside a
disposable RenkuOS VM. Their outputs matched the host artifacts byte for byte.
The native compiler is not yet integrated into `libvulkan.so`. Source
attribution and MIT license are retained in `native_rcs_shaders.c` and
`LICENSE.MIT`.

An [in-process native compiler API](../../../evidence/native-intel-compiler-api-005/README.md)
now accepts the same validated Vulkan 1.0 SPIR-V pair in memory and returns
the byte-identical N100 programs and selected Intel stage metadata. It is a
separate shared library; the Vulkan pipeline path still uses its known-good
fixture until runtime integration and hardware testing are complete.
