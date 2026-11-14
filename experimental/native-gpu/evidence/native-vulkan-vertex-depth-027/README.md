# Second N100 vertex program: compiler feasibility checkpoint

This is a compiler-only experiment for moving beyond the single fixed vertex
machine program. `triangle-depth-quarter.vert.spvasm` passes through each
vertex's x, y, and w while replacing z with 0.25. The original fragment
shader is unchanged. Host SPIRV-Tools assembled and validated both Vulkan 1.0
modules; the resulting SPIR-V binaries are retained here.

The current native Haiku Intel compiler library (SHA-256
`5079e911101181d922d1be6a9a91feb2856a8c7413b0ca172798e7964c8dce18`)
compiled both vertex modules on the VM using `compile_vertex.cpp`. The
[raw results](compiler-results.log)
show:

| Property | Existing vertex | Depth-quarter vertex |
|---|---:|---:|
| Machine-code bytes | 128 | 192 |
| Scratch bytes / relocations | 0 / 0 | 0 / 0 |
| GRF used / dispatch start | 128 / 2 | 128 / 2 |
| URB read length / entry size | 0 / 1 | 0 / 1 |
| Dispatch mode | 3 | 3 |
| VS SHA-256 | `cf08637755fa472911ca133f49e64693b5d1f460f2963dd5ec5e44dbb3cdab58` | `8cddb85cff2dbaed85ba7591abf60165cc01e4234d438dec54853f079a43df0a` |

The fragment machine code is byte identical in both compiles (208 bytes,
SHA-256 `b9f73778f45cfbd379a7f7d960fc047b5e75505231cc1356c718ff46b1b3e18b`).
The existing 128-byte result matches the pinned Mesa compiler proof. The new
192-byte program fits the existing 4 KiB owner vertex instruction page, and
the reported parameters used by the fixed `3DSTATE_VS` packet match the
existing program. This is evidence for a bounded owner experiment, not proof
that the new program is safe or renders correctly on hardware.

The native owner currently always fills the executable vertex page with the
128-byte program. A hardware test needs a new, independently validated owner
path that accepts an additional file-local vertex-shader BO, checks the entire
4 KiB BO against the exact approved program and zero padding, copies it into
the owner page before submission, and preserves the current version-7 path.
`render/RcsDepthVertex.h` contains the 48 machine dwords from `depth-vs.bin`
(byte-verified against this binary). `RcsPublicShader::ValidateVertexPage`
accepts only that exact 192-byte program with a zero-padded 4 KiB page. Its
CPU smoke passed the accepted page and rejected an instruction-bit change,
nonzero padding, and an incorrect page length. This validator is not yet wired
into the owner request path.
The Vulkan pipeline must upload the compiled vertex program and pass that BO
handle with each draw. Keep the current owner and Vulkan binaries as the
rollback checkpoint until a new path passes CPU and physical N100 regression
tests. No kernel or physical N100 state changed in this checkpoint.
