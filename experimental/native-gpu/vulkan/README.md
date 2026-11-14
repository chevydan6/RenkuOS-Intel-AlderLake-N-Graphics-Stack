# Native Vulkan ABI experiment

This direct-linked RenkuOS `libvulkan.so` uses Khronos Vulkan headers from the
pinned Mesa 26.3.0 archive. It currently implements instance and physical
device enumeration, one graphics device and queue, and shader-module object
lifetime. Physical device discovery probes the installed N100 owner through
the native execution bridge. No DRM/GEM, Linux loader, window system, base
kernel change, or Mesa runtime library is involved.

The library is intentionally kept in an isolated build bundle. Its reported
Vulkan 1.0 version is an **experimental API target**, not a conformance claim:
core pipeline, memory, command-buffer, synchronization, presentation, and
general shader compilation paths are still missing. Unsupported instance
extensions, device extensions, layers, and requested features are rejected.
Shader modules are copied and owned by the device; they are not executed yet.

## Reproduce on the N100

On the host, extract the exact 15 Khronos headers from the verified Mesa
26.3.0 archive into a new directory:

```sh
python3 experimental/native-gpu/vulkan/prepare-headers.py \
  /home/dan/projects/mesa-26.3-native/mesa-26.3.0.tar.xz \
  /home/dan/projects/new-native-vulkan-headers
```

The headers
retain Khronos copyright and `Apache-2.0 OR MIT` SPDX notices. Copy that
directory's `include/`, this directory's sources, and `../RenderReport.h` into
`/boot/home/native-vulkan-abi-001/` with the same layout, then run:

```sh
bash /boot/home/native-vulkan-abi-001/vulkan/build-native.sh
```

The physical test passed `vkCreateInstance`, `vkEnumeratePhysicalDevices`,
`vkGetPhysicalDeviceProperties`, queue discovery, `vkCreateDevice`, queue
creation, both SPIR-V shader modules, function-pointer lookup, and rejection
of a requested unsupported feature. [Build and closure evidence](../evidence/native-vulkan-abi-001/README.md)
records binary hashes and direct compile inputs.

`KnownSpirv.h` is generated from the validated Vulkan 1.2 assembly fixtures
in `../render/compiler/mesa-26.3.0/spirv/` using `generate-known-spirv.py` and
SPIRV-Tools. Its vertex and fragment module hashes match the Mesa compiler
proof. This fixture does not imply arbitrary SPIR-V support in RenkuOS.
