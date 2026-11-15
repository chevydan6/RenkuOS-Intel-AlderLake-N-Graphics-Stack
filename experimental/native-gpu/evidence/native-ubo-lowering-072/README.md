# First native UBO lowering experiment (072)

The [validated Vulkan UBO shader](../native-ubo-compiler-071/color-ubo.frag.spvasm)
translated to NIR containing `vulkan_resource_index`,
`load_vulkan_descriptor`, and `load_deref`. The [original NIR capture]
(translated-nir.log) records those operations. The current production compiler
adapter still rejects UBO shaders promptly; it was not changed here.

The [VM-only generator](make-probe-runtime.py) creates a one-binding compiler
experiment from the canonical adapter. It maps set 0/binding 0 to Intel binding
table entry 1, then runs Mesa NIR's `nir_lower_explicit_io` for UBO memory.
The [lowered NIR](lowered-bti1-nir-and-result.log) contains `load_ubo` with
index 1 and offset 0. Mesa 26.3's Intel backend compiled this to a 336-byte
fragment program, with SIMD8 and SIMD16 dispatch, zero scratch, and zero
relocations. The [machine code](lowered-bti1-fragment.bin) has SHA-256
`03179b37bcd77cf5399f298622c91e9a46d55dc8f79e1a518fb85fc729636f92`.
The known constant-color shader remained byte-identical
([control](known-bti1-control.log)). An earlier entry-0 experiment is also
saved; it is unsuitable for the current draw state because entry 0 holds the
render-target surface. Run [verify.py](verify.py) to check the saved outputs.

This does **not** prove UBO rendering on the N100. The current RCS surface
page contains only the render-target state, and `3DSTATE_PS` declares one
binding-table entry. The next hardware increment needs an actual UBO surface
state at entry 1, a two-entry binding table, descriptor/resource lifetime and
submission rules, and pixel verification. The one-binding NIR substitution
must then become a validated native descriptor-layout mapping before it is
enabled in the production compiler adapter.
