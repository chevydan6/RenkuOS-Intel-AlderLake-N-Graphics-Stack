# N100 compiler BLAKE3 dispatch, checkpoint 080

## Change

The pinned Mesa 26.3.0 compiler link now compiles Mesa's BLAKE3 dispatcher
with `BLAKE3_NO_AVX512`. The physical Intel N100 reports AVX2, SSE4.1, and
SSE2 but no AVX-512. The linked compiler retains those usable CPU paths.
The link script rejects an AVX-512 or original dispatcher archive object if
one is selected. No owner, bridge, Vulkan API, or shader source changed.

The candidate compiler is isolated at `/boot/home/native-blake3-080` on the
physical N100. The Vulkan library and probe were rebuilt from the same
`NativeVulkan.cpp` and `triangle_probe.cpp` sources as checkpoint 079 with
RPATH set to this candidate bundle. The owner and bridge hashes in
`physical-hashes.log` match checkpoint 079.

## Results

- The Haiku VM compiler pipeline probe passed its bounded RGB, vertex, and
  UBO cases. The same probe passed on the physical N100.
- The physical candidate passed 222 prior regression draws, 24 image-offset
  draws, and 16 buffer-offset draws. The image-offset runs covered 32×32 and
  64×64 direct and copy paths, UBO variants, and a rotated vertex shader.
  Logs and the exact reproduction scripts are in this directory.
- The selected compiler link fell from 346 to 345 objects and from 498 to
  497 Mesa source/include inputs. `report.json` records 11,890,881 selected
  Mesa source bytes, down from 12,052,862. The removed input is the 161,981
  byte AVX-512 assembly file. The linked compiler is 14,564,992 bytes, down
  29,280 bytes.
- `stack-manifest.json` measures **12,502,676 unique bytes** of selected
  canonical runtime source, down **161,981 bytes** from checkpoint 079's
  12,664,657 bytes. SDK, generated files, and probes are excluded. Broader
  kernel/build inputs and model-token counts remain unmeasured; the 3–5 MB
  objective is not met.

`compiler-bench-physical.log` contains ten interleaved 100-pair SPIR-V
compilation samples per library after warmup. The executables print their
loaded compiler paths; each output checksum is 48200. Baseline samples span
124,656–154,318 microseconds and candidate samples span 122,497–155,322.
The distributions overlap and vary with N100 CPU clock state, so this test
does not establish a speedup or slowdown.

## Reproduce

Build the compiler on the Haiku VM using `link-native-compiler.py` with Mesa
26.3.0, the checkpoint 075 runtime, native device-info/common overrides,
native GL header, and SPIR-V Tools library. `build.log` and `report.json`
record the isolated build. `vulkan-build.log` records the candidate Vulkan
library and probe build.

Run `run-image-offset.py`, `run-memory-offset.py`, and the five `regression/`
runner scripts from this directory with SSH access to `renkuos-n100`. The
scripts use `/boot/home/native-blake3-080` and write per-case evidence here.
`physical-hashes.log` identifies the exact binaries validated.
