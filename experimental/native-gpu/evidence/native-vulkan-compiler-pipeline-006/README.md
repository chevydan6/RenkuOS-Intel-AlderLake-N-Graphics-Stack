# Native Vulkan compiler pipeline, VM checkpoint 006

Date: 2026-11-15 (Asia/Tokyo). Branch: `codex/i915-native-adaptation`.

This experiment connects the native Mesa 26.3.0 Intel compiler API to the
RenkuOS Vulkan pipeline without changing the kernel owner or the default
hardware-tested build. The compiler-enabled pipeline accepts only the exact
Vulkan 1.0 SPIR-V pair previously tested on the N100, checks the compiled VS
and FS against the proven machine bytes, and uploads the newly compiled FS.

## Reproduce

In the disposable Haiku VM, stage `vulkan/`, `RenderReport.h`,
`ResourceCommand.h`, `render/draw/RcsDrawGenerated.h`, and
`render/compiler/mesa-26.3.0/IntelSpirvRuntime.h` with their repository paths
under a scratch directory. Use the compiler library built in
[`native-intel-compiler-api-005`](../native-intel-compiler-api-005/README.md).

```sh
RENKU_VULKAN_HEADERS=/boot/home/test-work/mesa-26.3.0/include \
RENKU_INTEL_COMPILER_LIB_DIR=/boot/home/test-work/build-compiler/src/intel/compiler/brw \
bash vulkan/build-compiler-pipeline.sh
```

The independent compiler probe compared both machine programs with the
generated N100 artifacts and rejected an altered fragment module and a short
output buffer. The Vulkan API probe called `vkCreateGraphicsPipelines` with a
synthetic device whose fd was `-1`. The accepted pair reached the owner
allocation call and returned `VK_ERROR_OUT_OF_DEVICE_MEMORY`; an altered pair
returned `VK_ERROR_FEATURE_NOT_PRESENT` before allocation. Both probes passed:

```text
compiler pipeline probe: matched hardware-tested fragment; rejected altered input and short buffer
Vulkan pipeline preflight: compiled accepted shader; rejected altered shader before owner call
3091f9553bd366aaed053785c4076c0f2469dea992da790f7875c11f23b87f77  libvulkan.so
a99d382d74e5d3d3de09f573be90bd215099fe90cceb2b68f6cbcc3ab9012c14  compiler_pipeline_probe
2cc6b4e1c813f8787725de3007753280918d3b6190902786b68b5fbba48ccda2  compiler_pipeline_api_probe
```

`libvulkan.so` dynamically needs `librenku_intel_compiler.so`,
`libstdc++.so.6`, `libgcc_s.so.1`, and Haiku `libroot.so`. The VM has no N100
PCI passthrough, and the physical N100 was unreachable during this checkpoint;
there is no new physical rendering result. The default fixed-program build
remains the known-good hardware path. This is a one-pair compiler integration,
not general Vulkan shader support or conformance.
