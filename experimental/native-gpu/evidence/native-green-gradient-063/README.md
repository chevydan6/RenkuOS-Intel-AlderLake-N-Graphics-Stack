# N100 native Vulkan green FragCoord gradient (063)

The native Vulkan path now accepts a second bounded fragment shader family:
`vec4(0, gl_FragCoord.x / D, 0, 1)` for `D = 32, 64, 128` on the 64×64
render target. The pinned Mesa 26.3.0 Intel compiler emits a 352-byte program
with the same dispatch metadata as the proven red gradient. Four instruction
words differ (indices 5, 28, 53, and 76); the scale immediates remain at
indices 29 and 77. [Compiler probe](compiler-diff-probe.cpp),
[red machine code](red-machine.bin), and [green machine code](green-machine.bin)
preserve the original 1/64 comparison. The green program is admitted only
when its complete zero-padded 4 KiB page matches that exact pattern and an
approved scale.

The owner add-on now recognizes the green instruction pattern and checks the
full completed image against a green-channel CPU reference. The Vulkan
compiler path accepts the matching SPIR-V program and the triangle probe
exposes `fragcoordgreen32`, `fragcoordgreen`, and `fragcoordgreen128`. The
base kernel, native execution bridge, and compiler binary were unchanged.
This remains a bounded shader family, not arbitrary Vulkan shader execution.

The Haiku VM [owner build](owner-vm-build.log) passed the complete CPU smoke
set, including new green shader-page and pixel-reference tests. The
[Vulkan build](vulkan-vm-build.log) passed compiler, API, fence, and command
reset probes; the [physical bundle build](native-candidate-build.log) used
the pinned 062 compiler. Before installing the new owner, the new green draw
was rejected at the owner boundary (`vk_queue_submit` returned `ioctl=-1`),
confirming that the new userspace path alone did not bypass the owner check.

On the physical N100, the installed owner is
`f4a4bce6b7fd5b45ef81867d3648b77a6690ee72fc2104aa2785b9defe557528`.
The previous owner is preserved at
`/boot/home/native-green-gradient-063/rollback/intel_memory_owner` with SHA-256
`12c5bac49dfc75089bd4d01a096b382c59f09ea8c88bb07b6ed9578f67817559`.
The [physical hash list](physical-hashes.log) records the owner, compiler,
Vulkan library, and probes. The N100 rebooted once to load the owner add-on.

The N100 passed **178 hardware draws across 54 cases** in the saved traces:

- [Prior direct-target matrix](physical-matrix.log): 39 cases, 124 draws.
- [Supplemental matrix](supplemental-matrix.log): 9 cases, 30 draws.
- [Prior fallback path](fallback.log): 2 cases, 8 draws.
- [New green scales](green-matrix.log): 3 direct-target cases and one fallback
  case, 16 draws. Each checked 3,476 pixels with no fault or ioctl failure.

Run `python3 verify.py` to audit the traces, sequence IDs, and expected image
hashes. The raw [green 1/64 image](green-pixels.bin) is byte-for-byte equal
to the [red 1/64 image](red64-reference.bin) with the varying red channel moved
to green. The [1/32](red32-reference.bin) and [1/128](red128-reference.bin)
red captures independently derive the other expected green hashes. The
[invalid shader run](invalid-shader-hardware.log) records four rejected owner
requests after the new owner was installed.

The [owner](owner-manifest.json), [Vulkan](vulkan-manifest.json), and
[combined stack](stack-manifest.json) manifests measure **12,629,415 unique
bytes of selected canonical runtime source**: 679 bytes above the 062
candidate. Generated compiler inputs, SDK headers, and other excluded source
remain separate. This is not a complete dependency-source or model-token
count; the 3–5 MB target and broad Vulkan compatibility are still open.
