# Vertex-to-fragment varying compiler experiment, checkpoint 105

The physical N100 indexed-UBO checkpoint proved the fragment shader can read
a descriptor, but the Vulkan driver still accepts only exact owner-approved
shader profiles. This experiment tests the next graphics capability without
altering the installed kernel owner or Vulkan library.

`varying.vert` reads a position at location 0 and a color at location 1,
then writes the color at output location 0. `varying.frag` reads that color
and writes it to the render target. The pinned glslang tool generated Vulkan
1.0 SPIR-V. `compile-varying.c` called the native Mesa 26.3.0 Intel compiler
library on the Haiku VM with SPIRV-Tools validation enabled. Compilation
succeeded:

| Stage | Code bytes | GRF | Other selected metadata |
|---|---:|---:|---|
| Vertex | 176 | 128 | dispatch GRF 2, URB read 0, URB entry 1, mode 3 |
| Fragment | 368 | 128 | SIMD8 and SIMD16, dispatch GRFs 4 and 6, SIMD16 offset 192 |

Both stages reported zero scratch and zero unresolved relocations. The exact
SPIR-V and machine-code binaries are saved here. Their SHA-256 values are:

| File | SHA-256 |
|---|---|
| `varying.vert.spv` | `4997f0a228a7c54910ebf9435ae75cb4fe9f002cee9e7fa2d59b406422d48a87` |
| `varying.frag.spv` | `eb2ba03683a1cf1131495e8bae035883e0bd793dbc1cf357eb33834b6fbf6661` |
| `varying.vs.bin` | `ddda1cbf11ca9112446fd4608cd4f7238242225ae70dbc1a8407d40f17915345` |
| `varying.fs.bin` | `eb70771336e5f28e9b9dc25c0625f8a03cb9a586444a0a6e609bccd6e5fd7b1c` |

The shader compiler can therefore produce this program without a new
compiler. The current driver cannot execute it: the vertex input is fixed
to one 16-byte attribute, `CompileKnownPipeline` rejects these machine-code
profiles, and the batch has no varying-specific SBE setup. Mesa ANV's
`genX_gfx_state.c` computes the `3DSTATE_SBE` and `3DSTATE_SBE_SWIZ` fields
from the VS VUE map and FS URB setup. The native compiler ABI currently
returns neither mapping. The next implementation step is to expose the
minimal linked varying metadata, configure two vertex elements and SBE from
it, then validate the owner-side shader and resource limits before a GPU
draw. A compiler success alone is not hardware proof.

The selected 14,839,534-byte runtime source estimate is unchanged by this
experiment. The largest selected Mesa input is the 1,362,950-byte Khronos
`vulkan_core.h`; it is shared with the native driver, so deleting its include
from one compiler source would not reduce whole-stack unique source bytes.
No source compaction was made while this capability gap remains open.
