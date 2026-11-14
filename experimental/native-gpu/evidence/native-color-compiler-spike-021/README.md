# Mesa compiled constant RGB on the physical N100

The pinned Mesa 26.3.0 Intel compiler was tested with Vulkan 1.0 fragment
modules that output constant RGBA. Alpha stayed at 1.0. The
[compiler matrix](compiler-matrix.log), produced by [color_matrix.cpp](color_matrix.cpp)
in the Haiku VM, showed that independent red, green, and blue changes can
preserve the proven 208-byte N100 instruction sequence. Only the paired
immediate words changed: red at 3/35, green at 7/39, blue at 11/43. The
compiler emitted the exact 128-byte vertex program, zero scratch, and zero
relocations for every matrix case.

The owner now validates that entire fragment page, including zero padding,
and permits paired finite nonnegative red/blue values through 1.0 and green
values below 1.0. It derives the expected BGRA8 pixel from the float bits.
The compiler-backed Vulkan library accepts only a compiled fragment that
passes this owner template check. Setting all three RGB values equal, or green
to 1.0, in the matrix
changed additional instructions, so those compiled modules are rejected by
the template gate. This is a machine-code shape gate, not general shader
support.

The Haiku VM owner build passed its full CPU smoke suite. The compiler-backed
Vulkan build passed its compiler, API, fence, and command reset probes. The
API probe accepted the distinct RGB module and rejected a 1.0-green module.

The physical N100 (PCI `8086:46d1`, revision 0) was installed with owner
SHA-256 `fdd85665ff0ee1989285fcf9e7796598529b94bf3d6ad8fb266c8a7232d2a075`.
The previous owner, SHA-256
`c5f72eb814f6e6169004870cd11a02c1b608be9f60b478947e7dfc3e47a0cf6b`,
is preserved at `/boot/home/native-rcs-rgb-021/rollback/intel_memory_owner.pre-rgb`.
The compiler-backed Vulkan library SHA-256 was
`ae1d74b463b5c699ebe3a8e5a22dfb3d35149a48107a4fc4310840a715579b0b`;
the final triangle application SHA-256 was
`da356407cc1df4ea530762bee808d000e68d9c593e44909ba49a0a23b8b3c41d`.

After a controlled reboot, the read-only RCS probe and bootstrap passed.
The new application rendered a distinct RGB color, red 0.5, green 0.75, blue
0.25, alpha 1.0. Its interior BGRA8 pixel was `ff80bf40`. The four exact
full-image hashes were:

| Shape | 0 | 1 | 2 | 3 |
|---|---|---|---|---|
| Hash | `6f2bc92d` | `a4e13743` | `ce445d05` | `fffe3dc5` |

One command buffer, fence, and BO set then passed 64 repeated RGB draws;
[raw trace](rgb-repeat-command-64.log), SHA-256
`2c2a297bd3e071b1c81aa5278c58f1f2fedfa9440a0aa7d15403bd49db0449d6`.
The final cross-color regression ran original, 0.5 green, 0.75 green, 0.9
green, and RGB across four shapes for eight passes: **160 exact image checks
and 160 completed submissions**, owner sequences 72–231, no reported GPU
faults. Every event had matching observed sequence and pixel counts. The
[cross-color trace](rgb-hardware-repeat-160.log) has SHA-256
`75226e01afcd831cd70c848ad009b9ff4f150dec5f06e3f391643208ef36f498`.

The workload still uses one fixed vertex shader, fixed 32×32 render target,
fixed graphics state and batch layout, synchronous submission, and CPU image
publication. More general Vulkan shaders and rendering remain open.
