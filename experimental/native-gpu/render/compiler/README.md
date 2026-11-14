# Offline N100 shader compiler

This directory preserves the Mesa 26.1.8 compiler and artifacts used by the
earlier RCS draw image. The separate `mesa-26.3.0/` recipe builds the current
shader bytes used by the physical N100 owner; see the hardware evidence in
`../../evidence/rcs-mesa263-shader-001/`.

This compiles actual Intel Gen12 VS and FS machine code with Mesa 26.1.8 BRW.
It creates NIR directly; no GL context, software rasterizer, GPU access or VM
modification occurs. It proves compilation only, not hardware execution.

Input: one vec4 position at `VERT_ATTRIB_GENERIC0` (15), copied to `gl_Position`.
Fragment output: constant linear RGBA `(1, .25, 0, 1)` at color region 0.
Device profile: Mesa's `intel_get_device_info_from_pci_id(0x46d1)` static N100
profile, graphics 12.0. No topology or revision is inferred from real hardware.

## Reproduce

On Ubuntu 24.04, with C/C++ compilers, Python3, curl, apt package metadata and
`dpkg-deb` already available:

```sh
./bootstrap-tools.sh /absolute/path/to/new-tools
source /absolute/path/to/new-tools/env.sh
./build.sh /path/to/mesa-26.1.8.tar.xz /absolute/path/to/new-build
```

The bootstrap downloads pinned Python tool versions and extracts just the dev
packages/runtime libraries required by this compiler into a private sysroot;
there are no privileged package installations. Package archives and checksums
remain in `new-tools/debs`. apt versions follow the configured Ubuntu archive;
retain those downloads to reproduce the exact dependency environment. The Mesa
archive SHA256 is checked before extraction. `build.sh` requires a fresh build
workspace and never edits an existing Mesa tree. It uses the archive's Meson
graph and builds only the requested executable with `idep_intel_compiler_brw`
and `idep_intel_dev`, including their declared dependencies. `integrate.py` is
the small version-checked Mesa integration recipe: enable the Intel subgraph
without driver selection, append the executable target, and copy the harness
and generated source identity header. No compiler objects are enumerated.

`build.sh` compiles twice into separate artifact directories under a clean
process environment and compares all five generated files byte for byte.
`SHA256SUMS` records output identities. An output directory must be new.

```sh
new-build/build/src/intel/compiler/brw/native-rcs-shaders NEW_OUTPUT_DIRECTORY
python3 check_artifacts.py NEW_OUTPUT_DIRECTORY
python3 check_artifacts_test.py -v
```

When invoking the executable directly, clear Intel compiler debug/tuning
environment variables as the build recipe does. The checked-in `artifacts/`
are the actual outputs from the verified recipe. Check the manifest with
`(cd artifacts && sha256sum -c SHA256SUMS)`.

## Artifact schema v1

All serialization explicitly writes scalar fields and integer arrays. No raw
Mesa structs, padding, host pointers or relocation records are exported.

| File / field | Meaning |
| --- | --- |
| `vs.bin`, `fs.bin` | Complete little endian BRW instruction streams; upload at hardware-required aligned addresses. Sizes include compiler padding. |
| `vs.nir`, `fs.nir` | Original builder NIR before preprocessing and stage compilation. |
| `metadata.json` root | Schema version, Mesa/archive/harness source identities, static device profile, vertex attribute and constant color. |
| stage common fields | Actual program byte size, scratch/shared usage, relocation count, constant data offsets/sizes, dispatch GRF start, GRF allocation, four push range byte sizes. |
| `vs.key` | Zero initialized `brw_vs_prog_key`; all fields zero. |
| `vs.inputs_read` | 64-bit hexadecimal mask, avoiding JSON integer precision loss. |
| `vs.urb_read_length` | Hardware URB read length, in 256-bit units. |
| `vs.urb_entry_size` | Hardware URB entry allocation, in 64-byte units. |
| `vs.dispatch_mode` | Mesa `intel_shader_dispatch_mode` enum; 3 is SIMD8. |
| `vs.vue_*`, maps | Actual VUE valid-slot mask, slot count and maps; `-1` means absent. Header-associated slots may map even when not in `slots_valid`. |
| `vs` additional fields | VUE handles, clip/cull masks, vertex/instance ID usage and four component-packing bitfields. |
| `fs.key` | Zero initialized `brw_fs_prog_key` except `nr_color_regions=1`, `multisample_fbo=INTEL_NEVER` (0). Compile parameter `max_polygons=1`, `use_rep_send=false`. |
| `fs.dispatch_*` | Compiled SIMD variants; 0/1 flags. SIMD8 uses offset zero. Other offsets are bytes from `fs.bin` start, meaningful only when enabled. |
| `fs` input fields | Varying input count/mask, barycentric interpolation mask, flat inputs, all `urb_setup` slots. |
| `fs` additional fields | Actual depth/stencil/kill/omask/source depth/source W/side-effect flags and per-sample/coarse dispatch enums. |

The executable rejects compile failures, zero code, scratch and unresolved
relocations before writing metadata. The checker also requires nonzero code,
matching byte sizes, the exact source hash, supported stage layout, zero
constant/shared/push storage, the exact supported VUE position/header mapping, and distinct enabled dispatch offsets. This checker
validates the supported initial shader profile; it is not a general shader or
GPU command validator and these fields alone do not form a render state image.

The compile sequence, VS VUE setup and NIR variable approach follow pinned
Mesa `src/intel/blorp/blorp_brw.c`. Intel attribution and MIT license are
preserved in the harness and `LICENSE.MIT`.
