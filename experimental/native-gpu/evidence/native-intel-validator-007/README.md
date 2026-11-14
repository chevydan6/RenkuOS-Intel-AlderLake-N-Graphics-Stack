# Native Vulkan 1.0 shader validation, VM checkpoint 007

Date: 2026-11-15 (Asia/Tokyo). Branch: `codex/i915-native-adaptation`.

The in-process Mesa 26.3.0 Intel compiler now calls SPIRV-Tools' Vulkan 1.0
binary validator before SPIR-V-to-NIR translation. The public C ABI exposes
`renku_intel_validate_vulkan10` for a future Vulkan shader-module boundary.
The compiler still supports only the currently configured N100 vertex and
fragment compilation path; validation alone does not establish general shader
support or Vulkan conformance.

## Native VM build

The disposable Haiku VM used HaikuPorts packages
`spirv_tools-1.4.350.0-1-x86_64.hpkg` and
`spirv_tools_devel-1.4.350.0-1-x86_64.hpkg` (pkg-config version 2026.2.1).
The downloaded HPKG SHA-256 values were:

```text
86b129653247d613e5da66f2dff1d23c2f57333cf9b712532d0629c3b9a265fe  spirv_tools-1.4.350.0-1-x86_64.hpkg
9499e21f4742a134773dc9c058e96f5bac1dd6abbc7091f2111ebc279c2faef4  spirv_tools_devel-1.4.350.0-1-x86_64.hpkg
```

The package files were staged under `/boot/home/test-work`; `pkgman install
-H -R` activated them in the VM home package set. The compiler was configured
with `-Dspirv-tools=disabled` for Mesa's optional paths, while the native
shared library has its own explicit `SPIRV-Tools-shared` dependency. The final
ELF needs `libSPIRV-Tools-shared.so`, `libstdc++.so.6`, `libgcc_s.so.1`, and
Haiku `libroot.so`. It does not need libdrm.

## Results

`runtime_smoke.c` passed a valid vertex module, rejected a header-valid module
with a zero-word instruction, and confirmed that compilation also rejects it
before NIR translation. It recompiled the known vertex and original fragment
pair, then the half-green fragment variant. The machine code stayed byte
identical to prior compiler and physical N100 artifacts:

```text
validator rejected malformed SPIR-V: SPIR-V word 1: Invalid instruction word count: 0
native Intel compiler API: VS=128 FS=208
cf08637755fa472911ca133f49e64693b5d1f460f2963dd5ec5e44dbb3cdab58  vs-validated.bin
b9f73778f45cfbd379a7f7d960fc047b5e75505231cc1356c718ff46b1b3e18b  fs-validated.bin
ea9fadfaeaa943979f55e30be1b48dc0732b041061699819d34f2bca2e7ef2aa  fs-half-validated.bin
```

The compiler-enabled Vulkan pipeline probes also passed against this new
library. A swapped vertex/fragment pair returned a translation error rather
than crashing. There was no N100 hardware test because the physical machine
remained unreachable.

The final compiler shared library is 14,613,120 bytes, SHA-256
`5079e911101181d922d1be6a9a91feb2856a8c7413b0ca172798e7964c8dce18`.
Its SPIRV-Tools runtime dependency is 2,086,176 bytes. These are ELF sizes,
not source-code or AI-token counts. The earlier measured Mesa source closure
was 14,025,279 bytes before this dependency; the updated complete source
closure, including SPIRV-Tools, has not yet been measured. No source-size
optimization was attempted in this checkpoint.
