# Offline compiler verification, 2026-11-14

Executed on Ubuntu 24.04, GCC/G++ 13.3.0; no privileged package installs,
GPU device access, native VM changes or edits to existing Mesa trees.

```sh
cd /home/dan/projects/i915-native-validation/experimental/native-gpu/render/compiler
./bootstrap-tools.sh /home/dan/projects/native-rcs-compiler-001/final-tools
source /home/dan/projects/native-rcs-compiler-001/final-tools/env.sh
./build.sh /home/dan/vms/renkuos-mesa25/mesa-26.1.8.tar.xz \
    /home/dan/projects/native-rcs-compiler-001/final-build
python3 check_artifacts.py artifacts
(cd artifacts && sha256sum -c SHA256SUMS)
```

Results:

- Archive checksum matched the pinned `b320f658...dc0b55`.
- Fresh Meson graph built `native-rcs-shaders` successfully, 627 build steps,
  with no compiler warnings/errors. No GL or Vulkan driver was selected.
- The checker first failed on the nonexistent artifact directory, before any
  compiled output existed. It passed the actual compiled artifacts afterward.
- Two compiler invocations produced byte-identical binaries, original NIR and
  pointer-free metadata. The source identity is SHA256 of the harness itself.
- VS: 176 bytes, SIMD8, dispatch GRF start 2, URB read length 1, URB entry size 1,
  VUE slots 2, position input bit 15. `vs.bin` SHA256:
  `ae634c7ff83ed365bb27acc40161c2047995f7e274c5bba014497324bc1bc614`.
- FS: 112 bytes, SIMD8+SIMD16, SIMD16 byte offset 64, dispatch GRF starts 2,
  zero varying inputs and barycentrics. `fs.bin` SHA256:
  `58bacb3bdebdc9cafb06b15373580fa0e01adc4eb4c6077214a0aec6eb1ee23d`.
- Both programs: zero scratch, unresolved relocations, shared memory, constant
  data and push storage. All five artifact checksums passed.
- Checker negative mutations rejected nonzero scratch, a relocation, an invalid
  SIMD16 offset and all-zero code. Python `-O` still rejects invalid artifacts.
- Shell syntax and Python compilation checks passed. Self-review confirmed
  explicit scalar/array serialization and Mesa declared dependency use.

Private build logs remain under `/home/dan/projects/native-rcs-compiler-001/`:
`bootstrap.log`, `final-build.log`. Exact apt archives/checksums remain in
`final-tools/debs`; the compiler executable is
`final-build/build/src/intel/compiler/brw/native-rcs-shaders`.

Compilation and artifact checks do not prove GPU execution or render state
correctness. Native shader drawing remains a later task.

## Focused checker review correction

Review identified two metadata corruptions previously accepted: position VUE
slot `varying_to_slot[0]=999`, and enabled SIMD16 offset changed from 64 to 0,
aliasing enabled SIMD8. Persistent regressions use copies of the real artifacts
and invoke the checker with Python `-O`.

RED, before tightening the checker:

```sh
python3 /home/dan/projects/i915-native-validation/experimental/native-gpu/render/compiler/check_artifacts_test.py -v
```

Result: 4 tests run; `test_position_vue_slot_out_of_range` and
`test_simd16_aliases_simd8` failed because the checker returned success. The
actual-artifact test and existing negative-profile test passed.

GREEN, after tightening the checker, using the same command: 4 tests passed.
The checker now requires this shader's exact supported Mesa VUE layout:
valid mask 1, two slots `[PSIZ(12), POS(0)]`, position mapped to slot 1,
header/aliases mapped to slot 0, and all remaining varyings absent. The FS
setup remains entirely absent because it has no varying inputs. Enabled SIMD
variants must have distinct code offsets, including implicit SIMD8 offset 0.

Rechecked `python3 check_artifacts.py artifacts`: PASS. Rechecked
`(cd artifacts && sha256sum -c SHA256SUMS)`: all five outputs OK. The harness
and generated artifacts are unchanged; no compiler rebuild was required.
