# Compiler output for more constant-green Vulkan shaders

Date: 2026-11-15 (Asia/Tokyo). The disposable RenkuOS VM's workspace
passed `checkfs --check-only` with 15,388 nodes and zero blocks to free.
Fresh canonical Vulkan sources rebuilt all four compiler-backed CPU/API
probes. The pinned `librenku_intel_compiler.so` retained SHA-256
`5079e911101181d922d1be6a9a91feb2856a8c7413b0ca172798e7964c8dce18`.

The [throwaway matrix probe](constant_color_matrix.cpp) copied the valid
half-green SPIR-V module, changed its sole `0.5` constant, and called
`renku_intel_compile_vulkan_pair` for seven values. It compared each compiled
fragment program with the known 208-byte N100 program, excluding only the two
green immediate words at indexes 7 and 39. The [raw results](compiler-matrix.log)
show identical vertex code, zero scratch and relocations, and no other
fragment word differences for `0.0`, `0.1`, `0.25`, `0.5`, `0.75`, and `0.9`.
Both immediate words equal the requested float bits. For `1.0`, 12 other
fragment words differ, so the current fixed packet template cannot admit
that compiled program through an immediate-only check.

This is a compiler-only finding. The installed owner and Vulkan driver still
admit only the two hardware-tested exact shader modules. A next increment can
validate the compiled machine-code template and the owner-rendered pixel for
values below `1.0`, then test actual N100 output before widening acceptance.
No GPU command was submitted by this probe.
