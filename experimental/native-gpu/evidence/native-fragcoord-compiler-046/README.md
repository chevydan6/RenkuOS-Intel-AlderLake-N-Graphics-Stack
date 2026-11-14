# FragCoord fragment compiler probe (046)

This experiment identifies the next shader execution boundary after the N100 constant-RGB triangle. The MIT-licensed `fragcoord-red.spvasm` reads Vulkan `FragCoord.x`, multiplies it by `1/64`, and writes that value to the red output channel. SPIRV-Tools v2025.1 assembled and validated the 404-byte module for Vulkan 1.0. The native Mesa 26.3.0 compiler produced a 336-byte Intel N100 fragment program with no scratch use or relocations. Both the Haiku VM and the physical N100 produced the same machine-code SHA-256, `bdc1cd20b6a63214273d2cf40ee0f6a21a798a332d7424f0c23e9729d1b4dea2`.

The physical compiler result is in `physical-compile.log`; exact SPIR-V and machine bytes are retained in `fragcoord-red.spv` and `fragcoord-red.fs.bin`. The probe uses the existing known vertex SPIR-V and calls `renku_intel_compile_vulkan_pair` through the native compiler library from candidate 045.

The compiler reports SIMD8 and SIMD16 dispatch, a SIMD16 offset of **192 bytes**, and dispatch start GRFs 4 and 6. The current owner admits only a 208-byte constant-color instruction template and patches the SIMD16 pointer at a fixed **128-byte** offset. The Vulkan pipeline's `CompileKnownPipeline` gate also requires that template. A correct end-to-end implementation therefore needs to admit and validate this new instruction program, carry its compile metadata into PS state, and verify its varying pixel output. This probe did **not** submit the new program to the GPU and makes no rendering claim.

On the host, assemble and validate with the pinned SPIRV-Tools v2025.1 binary:

```sh
/home/dan/projects/mesa-26.3-native/anv-tools/usr/bin/spirv-as --target-env vulkan1.0 experimental/native-gpu/render/compiler/mesa-26.3.0/spirv/fragcoord-red.spvasm -o fragcoord-red.spv
/home/dan/projects/mesa-26.3-native/anv-tools/usr/bin/spirv-val --target-env vulkan1.0 fragcoord-red.spv
```

On the Haiku VM, copy `compile-fragcoord.cpp` and `fragcoord-red.spv` to `/boot/home/test-work/fragcoord-046/`, then build and run:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I/boot/home/test-work/closure-current-022/src -I/boot/home/test-work/compiler-pipeline-restored-008/vulkan /boot/home/test-work/fragcoord-046/compile-fragcoord.cpp -L/boot/home/test-work/native-compiler-045 -lrenku_intel_compiler -o /boot/home/test-work/fragcoord-046/compile-fragcoord
LIBRARY_PATH=/boot/home/test-work/native-compiler-045:/boot/home/test-work/compiler-pipeline-restored-008/lib:/boot/home/config/lib:/boot/system/lib /boot/home/test-work/fragcoord-046/compile-fragcoord /boot/home/test-work/fragcoord-046/fragcoord-red.spv /boot/home/test-work/fragcoord-046/fragcoord-red.fs.bin
```

The N100 stage location was `/boot/home/native-fragcoord-046/`. Its compiler binary was staged separately from the known-good owner and Vulkan installation.
