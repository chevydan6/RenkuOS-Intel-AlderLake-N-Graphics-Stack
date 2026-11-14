# Native Vulkan ABI experiment

This direct-linked RenkuOS `libvulkan.so` uses Khronos Vulkan headers from the
pinned Mesa 26.3.0 archive. It currently implements instance and physical
device enumeration, one graphics device and queue, owner-backed memory,
fixed-shape render target objects, and one bounded graphics pipeline and draw
path. Physical device discovery probes the installed N100 owner through the
native execution bridge. No DRM/GEM, Linux loader, window system, base kernel
change, or Mesa runtime library is involved in the default hardware-tested
build. A separate compiler-backed build is described below.

The library is intentionally kept in an isolated build bundle. Its reported
Vulkan 1.0 version is an **experimental API target**, not a conformance claim.
It accepts only the two SPIR-V fixtures and a fixed N100 pipeline and command
shape. General shader compilation, resource formats, command streams,
semaphores, fences, presentation, and broad Vulkan compatibility remain open.
Unsupported instance extensions, device extensions, layers, and requested
features are rejected.
Device creation bootstraps the existing RCS owner. Vulkan memory allocation
creates an owner BO, and explicit flush/invalidate copies mapped staging data
through the native resource ioctl. This memory type is host-visible and
noncoherent. The native interface currently supports one 64 KiB heap,
vertex-buffer usage, and one 32×32 linear BGRA color image shape. A synchronous
`vkQueueSubmit` submits one owner-validated hardware triangle and copies the
completed pixels into the bound image BO. Set `RENKU_VK_TRACE=1` for a concise
JSON submission report; normal draws skip the diagnostic print.

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
directory's `include/`, this directory's sources, both `../RenderReport.h` and
`../ResourceCommand.h`, and `../render/draw/RcsDrawGenerated.h` with its
`LICENSE-MIT-Intel.txt` into `/boot/home/native-vulkan-triangle-005/` with the
same layout, then run:

```sh
bash /boot/home/native-vulkan-triangle-005/vulkan/build-native.sh
```

The physical test passed `vkCreateInstance`, `vkEnumeratePhysicalDevices`,
`vkGetPhysicalDeviceProperties`, queue discovery, `vkCreateDevice`, queue
creation, both SPIR-V shader modules, function-pointer lookup, and rejection
of a requested unsupported feature. The current probe also allocates and binds
a vertex buffer, flushes known triangle vertices into its owner BO, invalidates
and checks the exact readback, and rejects a one-byte allocation for that
buffer. [Memory checkpoint evidence](../evidence/native-vulkan-memory-002/README.md)
records physical repetitions, binary hashes, and direct compile inputs.
The [image checkpoint](../evidence/native-vulkan-image-003/README.md) verifies
the fixed output-image allocation and binding path.
The [render-target checkpoint](../evidence/native-vulkan-target-004/README.md)
verifies the image-view, render-pass, and framebuffer chain for that shape.
The [combined owner checkpoint](../evidence/rcs-combined-015/README.md) now
allows one hardware draw to consume both public vertex and shader BOs. Vulkan
pipeline and command submission now reach that request. The
[Vulkan triangle evidence](../evidence/native-vulkan-triangle-005/README.md)
records four image hashes, repeated physical draws, structured trace, and
direct compile-input sizes.

`KnownSpirv.h` is generated from the validated Vulkan 1.0 assembly fixtures
in `../render/compiler/mesa-26.3.0/spirv/` using `generate-known-spirv.py` and
SPIRV-Tools. Its vertex and fragment module hashes match the Mesa compiler
proof. This fixture does not imply arbitrary SPIR-V support in RenkuOS.

## Compiler-backed pipeline experiment

`build-compiler-pipeline.sh` builds an alternate `libvulkan.so` on Haiku with
the native Mesa 26.3.0 Intel compiler library. Set `RENKU_VULKAN_HEADERS` to
the extracted Khronos header directory and `RENKU_INTEL_COMPILER_LIB_DIR` to
the directory containing `librenku_intel_compiler.so`. The script also builds
and runs two probes. The original `build-native.sh` remains the physical N100
regression path.

The alternate pipeline compiles the accepted Vulkan 1.0 SPIR-V pair during
`vkCreateGraphicsPipelines`, verifies both machine programs against the
hardware-tested bytes, and uploads the compiled fragment program. It rejects
all other SPIR-V before calling the compiler because the small compiler API
does not yet provide full SPIR-V validation and the owner accepts one exact
fragment program. The VM test reaches owner allocation with the accepted pair
and rejects an altered pair before that call. It does not execute GPU work;
physical N100 rendering still needs validation before this build can replace
the fixed-program path. [VM evidence](../evidence/native-vulkan-compiler-pipeline-006/README.md).
