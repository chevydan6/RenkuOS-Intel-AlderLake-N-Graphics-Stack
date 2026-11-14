# Native Vulkan second shader preflight, VM checkpoint 014

Date: 2026-11-15 (Asia/Tokyo). Branch: `codex/i915-native-adaptation`.

The generated `KnownSpirv.h` now includes the validated Vulkan 1.0
half-green fragment module, SHA-256
`9620bd53ca2723ec6c04427f28308ce076d23f10f3be155cf500099985fdb4f5`.
The compiler-backed Vulkan build admits this module alongside the original
fragment fixture. `CompileKnownFragment` recompiles each exact SPIR-V pair and
checks all VS/FS bytes and the supported metadata before creating the shader
resource. The half-green output differs at only the two known Intel immediate
words. Other shader pairs remain rejected before owner allocation.

The compiler pipeline probe initially exited 5 when the half-green fixture
was rejected. After opening its bounded compiler path, the Vulkan API probe
exited 6 because pipeline creation still rejected it. Both probes then passed
after that gate was updated. The compiler-backed VM build passed all four
probes and compiled `triangle_probe` for a later physical test:

```text
compiler pipeline probe: matched original and half-green machine code; rejected altered input and short buffer
Vulkan API probe: validated both modules; compiled both pipelines; rejected altered pipeline
Vulkan fence probe: timed wait; empty submit signal; reset; signaled creation; wait-any/all
Vulkan command reset: pool, explicit buffer, and implicit begin reuse
```

The VM compiler-backed `libvulkan.so` SHA-256 is
`56b8f01425dd9086145ab40d1c49385f5198a48a7997185ce154dcb70d6c1788`.
The compiled half-capable `triangle_probe` hashes
`e1abb5fa9308733c0f8e30b7273a8293d6550173cfc00d6110ae32343257be77`.
The default noncompiler `libvulkan.so` and triangle probe also compiled with
`-Werror`; that build still accepts only the original shader pair. Its library
hash is `cedcaa0136a546475fee2b2a106aee77cca331deeca493e22bc943cc00a3012e`.

`triangle_probe SHAPE half` is prepared to request a half-green draw and check
an interior pixel, untouched border, and changed image hash. Its four full
image hashes will be recorded after actual hardware runs. Neither the
compiler-backed Vulkan library nor the matching
[two-shader owner](../native-rcs-two-shaders-013/README.md) was installed on
the physical N100 in this checkpoint. The original physical bundle remains
the regression baseline.
