# Vulkan SPIR-V to Intel code proof

`triangle.vert.spvasm` and `triangle.frag.spvasm` are minimal Vulkan 1.0
shaders: location 0 position passes to `gl_Position`, and the fragment shader
writes RGBA `(1, .25, 0, 1)`. `prove.py` assembles and validates both modules,
compiles `compile_spirv.c` against the same pinned Mesa 26.3.0 build graph as
the direct-NIR harness, and compares every shader metadata field and both
machine-code binaries with the artifacts that passed physical N100 rendering.
It builds into a new host directory and leaves the Mesa tree and installed
N100 driver untouched.

```sh
source /home/dan/projects/native-rcs-compiler-001/final-tools/env.sh
python3 experimental/native-gpu/render/compiler/mesa-26.3.0/spirv/prove.py \
  /home/dan/projects/native-rcs-compiler-263-final \
  /home/dan/projects/mesa-26.3-native/anv-tools/usr/bin \
  /home/dan/projects/new-native-spirv-proof
```

The output directory must not exist. The current [proof](proof.json) passed
with SPIRV-Tools v2025.1 (`spirv-as` SHA-256
`ef27f4a3890f80abd21bd9cb919b16471472bcbb94dfa6f455b6bb3f1f047bdd`,
`spirv-val` SHA-256
`20c5998a46f02310f56bf11be238c34d374bb50104b7ccf85bb909f9da883ff9`).
The assembled VS and FS modules were valid for Vulkan 1.0. The Intel compiler
emitted the same 128-byte VS and 208-byte FS bytes as direct NIR. A negative
control changed the fragment constant from `.25` to `.5`: the VS remained
byte-identical while the FS changed to SHA-256
`ea9fadfaeaa943979f55e30be1b48dc0732b041061699819d34f2bca2e7ef2aa`.

This proves the minimal SPIR-V → NIR → Intel machine-code path for these two
shaders. It does not demonstrate arbitrary Vulkan shaders, pipeline layouts,
descriptor lowering, dynamic vertex formats, native userspace compilation, or
Vulkan API calls. The compiler proof compares the identical machine-code
bytes with the direct-NIR harness; the later native Vulkan triangle experiment
exercises those bytes through a Vulkan command buffer.

An earlier [Vulkan 1.2 proof](proof-vulkan12.json) used SPIR-V 1.5 modules.
`spirv-val --target-env vulkan1.0` rejected those modules because their binary
version exceeded SPIR-V 1.0. Reassembling the same sources with the Vulkan
1.0 target changes only one byte in each module's version word. The current
proof confirms both modules still compile to the exact same hardware-tested
VS/FS bytes. The native Vulkan triangle application now uses these Vulkan
1.0 modules.
