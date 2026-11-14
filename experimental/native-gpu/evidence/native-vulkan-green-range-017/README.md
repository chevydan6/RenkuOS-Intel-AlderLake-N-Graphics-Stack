# Native Vulkan bounded constant-green shader, physical N100

Mesa 26.3.0 compiled Vulkan 1.0 SPIR-V for the pinned vertex shader and
constant-green fragment shader. The compiler-backed Vulkan path now accepts a
fragment module only when the emitted 208-byte N100 program matches the owner's
fixed instruction template, apart from two equal float immediates below 1.0.
The native owner independently validates the same instruction shape and derives
the expected BGRA8 pixel using integer float-bit conversion. The fixed batch,
vertex shader, pipeline state, and 32×32 render target remain required.

VM preflight: the owner `build-execution.sh` passed, including the bounded
shader CPU probe. The compiler pipeline build passed its compile/API, fence,
and command reset probes. The compiler/API probes accepted 0.75 and rejected
1.0 (which changes other machine instructions), malformed SPIR-V, and a short
output buffer.

On the physical N100 (PCI `8086:46d1`, revision 0), the previous installed
owner SHA-256 was `a00cbf15ae53c07b0f0f2214a60fd1cbe28c15b448e4e7fa992a5e7e208b7b42`.
Its rollback copy is
`/boot/home/native-rcs-green-range-016/rollback/intel_memory_owner.pre-green-range`.
After a controlled reboot, the installed owner SHA-256 was
`c5f72eb814f6e6169004870cd11a02c1b608be9f60b478947e7dfc3e47a0cf6b`.
The alternate Vulkan library SHA-256 was
`098358d81e35782a07b1ef45eb693d855f521d047a647ba87cea53c269c27964`.
The final triangle probe SHA-256 was
`192edce5b5a1a76c7d29c199eadf158e230be79a501f09ae6fde7e2019bcf55e`.

The read-only RCS probe and explicit bootstrap passed after reboot. Four
shapes for each of the original, 0.5, 0.75, and 0.9 green shaders passed eight
times each: **128 exact image hashes and 128 completed version-7 submissions**.
Every trace event reports `ioctl=0`, `stage=12`, `fault=0`, matching checked and
matched pixel counts, and an observed sequence equal to the submitted one.
The final log is [hardware-repeat-128.log](hardware-repeat-128.log), SHA-256
`e798e199d749638963f26816c76c73b1f486b73e2eb9c8f8aa72d35fc68932c4`.

| Green | Shape 0 | Shape 1 | Shape 2 | Shape 3 |
|---|---|---|---|---|
| 0.25 | `28616d2d` | `d40eb9c3` | `e24aed05` | `984ae945` |
| 0.5 | `1629ed2d` | `0bf039c3` | `48e0ed05` | `05d66945` |
| 0.75 | `28e87f2d` | `a6f78dc3` | `cef9f105` | `abd7c545` |
| 0.9 | `979c9f2d` | `6a3fddc3` | `44297d05` | `d034dd45` |

The 0.9 interior pixel was `ffffe500`: IEEE binary32 0.9 is slightly below
decimal 0.9, so the green UNORM8 channel rounds to 229 (`e5`). This corrected
an earlier CPU test expectation of 230.

This is a bounded compiler-driven shader family, not general Vulkan shader or
pipeline support. Presentation, broader formats, asynchronous submission, and
conformance remain open.
