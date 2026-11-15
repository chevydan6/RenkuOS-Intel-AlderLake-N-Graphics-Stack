# Vulkan UBO compiler boundary (071)

A valid Vulkan 1.0 [fragment shader](color-ubo.frag.spvasm) reads one RGBA
value from a uniform buffer at set 0, binding 0. The pinned SPIR-V Tools
assembler and validator accepted the [binary](color-ubo.spv), SHA-256
`685ab4a8736ce43270bdd6f60f7f6e393c65f3d422f4d34761aa458d9e33cef1`.
The existing native Mesa 26.3 compiler adapter validated it, then did not
return from code generation within five seconds on the Haiku VM
([reproduction](old-ubo-repro.log)). The known constant-color shader compiled
immediately in the same [probe](compiler-probe.cpp).

Mesa ANV's shader path lowers descriptor layouts, explicit UBO addresses, and
UBO loads before calling `brw_compile` (`anv_shader_compile.c`, around the
`anv_nir_apply_pipeline_layout`, `nir_lower_explicit_io`, and
`anv_nir_lower_ubo_loads` passes in Mesa 26.3.0). The narrow RenkuOS adapter
does not yet provide that path. This missing lowering is the likely cause of
the stall; the experiment does not isolate the exact internal loop.

The adapter now returns `RENKU_INTEL_COMPILE_UNSUPPORTED` with an explicit
descriptor-lowering message when NIR reports a UBO. The isolated candidate
returned promptly on the [VM](candidate-ubo-vm.log) and [physical N100]
(ubo-rejection-n100.log). Its [known shader output](candidate-known-vm.log)
remained byte-identical to the existing 208-byte machine program (SHA-256
`b9f73778f45cfbd379a7f7d960fc047b5e75505231cc1356c718ff46b1b3e18b`).

The physical N100 candidate compiler passed [206 prior draws]
(regression/verify.py) across 68 cases, plus the arena runner's 24 draws.
The [first physical draw](first-draw-n100.log), [compiler build]
(compiler-build.log), [binary hashes](physical-hashes.log), and [selected
source manifest](stack-manifest.json) are saved. Run [verify.py](verify.py)
to check the captured evidence. The installed owner and existing Vulkan
library were unchanged; this candidate compiler is isolated at
`/boot/home/native-ubo-compiler-071`.

Selected canonical runtime source is **12,637,741 unique bytes**, 370 above
checkpoint 070. Model tokens remain unmeasured. UBO rendering still requires
native descriptor layout and binding-table work; this increment makes that
missing path fail quickly and diagnosably.
