# Native Vulkan ABI experiment

This direct-linked RenkuOS `libvulkan.so` uses Khronos Vulkan headers from the
pinned Mesa 26.3.0 archive. It currently implements instance and physical
device enumeration, one graphics device and queue, owner-backed memory,
fixed-shape render target objects, and bounded graphics pipelines and draws
path. Physical device discovery probes the installed N100 owner through the
native execution bridge. No DRM/GEM, Linux loader, window system, base kernel
change, or Mesa runtime library is involved in the default hardware-tested
build. A separate compiler-backed build is described below.

The library is intentionally kept in an isolated build bundle. Its reported
Vulkan 1.0 version is an **experimental API target**, not a conformance claim.
The hardware-tested default build accepts one vertex and one fragment SPIR-V
fixture and a fixed N100 pipeline and command shape. General shader execution,
resource formats, command streams,
semaphores, presentation, and broad Vulkan compatibility remain open. Basic
host fences now support creation, status, reset, wait, and signaling after a
successful synchronous queue submission. A physical N100 triangle probe now
tests fence state before submission, after GPU completion, and after reset.
The compiler-backed path accepts multiple submit records and command buffers
without semaphores; it validates the full call before executing commands in
order and signals the fence after the final successful draw. A two-submit call
and the original single-submit path passed on all four triangle shapes.
It now accepts multiple graphics pipelines in one creation call, validates
their fixed state before allocation, and rolls back earlier pipelines if a
later allocation fails. The [N100 dual-pipeline checkpoint](../evidence/native-vulkan-multipipeline-031/README.md)
records two distinct fragment programs submitted in order, resource-exhaustion
rollback, and the final pixel readback.
Command buffers can now record multiple render-pass instances, each containing
one bounded draw. Submission replays their captured framebuffer, pipeline, and
vertex bindings in order and signals the fence after the last draw. The
[multi-pass checkpoint](../evidence/native-vulkan-multipass-032/README.md)
records two draws from one command buffer on the N100. Multiple draws within
one render pass and attachment load/preservation semantics remain unsupported.
The [batch checkpoint](../evidence/native-vulkan-batch-025/README.md) records
the exact output hashes and hardware trace.
`vkResetCommandPool` now returns allocated command buffers to the initial
state for reuse; that path has also only been tested in the VM.
Pools created with `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT` also
support `vkResetCommandBuffer` and implicit reset when recording begins again.
Unsupported instance extensions, device extensions, layers, and requested
features are rejected.
Device creation bootstraps the existing RCS owner. Vulkan memory allocation
creates an owner BO, and explicit flush/invalidate copies mapped staging data
through the native resource ioctl. This memory type is host-visible and
noncoherent. The native interface currently supports one 64 KiB heap,
vertex-buffer usage, and one 32×32 linear BGRA color image shape. A synchronous
`vkQueueSubmit` submits each owner-validated hardware triangle and copies its
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
SPIRV-Tools. Its original vertex and fragment module hashes match the Mesa
compiler proof. A half-green fragment fixture is used only by the alternate
compiler-backed build. These fixtures do not imply arbitrary SPIR-V support.

## Compiler-backed pipeline experiment

`build-compiler-pipeline.sh` builds an alternate `libvulkan.so` on Haiku with
the native Mesa 26.3.0 Intel compiler library. Set `RENKU_VULKAN_HEADERS` to
the extracted Khronos header directory and `RENKU_INTEL_COMPILER_LIB_DIR` to
the directory containing `librenku_intel_compiler.so`. The script builds and
runs four CPU/API probes and compiles a hardware triangle probe. The original
`build-native.sh` remains a known-good N100 regression path.

The alternate pipeline compiles Vulkan 1.0 SPIR-V during
`vkCreateGraphicsPipelines`, requires the exact owner-approved vertex machine
program, and uploads a
fragment program only when its 208-byte machine code matches the bounded N100
constant-RGB instruction template. The native owner independently validates
that template, including paired red, green, and blue immediates and the entire
zero-padded shader page. Red and blue are bounded to `[0, 1]`; green is bounded
to `[0, 1)`. Other machine code and pipeline states are rejected.
The compiler-backed build validates modules for Vulkan
1.0 with SPIRV-Tools at `vkCreateShaderModule`, before pipeline creation. The
vertex SPIR-V bytes may differ when the compiler emits that same machine
program; an added `OpName` module passed the VM gate and the physical N100 on
all four triangle shapes. [Named vertex evidence](../evidence/native-vulkan-named-vertex-026/README.md)
records the exact trace and image hashes. The
API probe rejects a semantically malformed module at that call, reaches owner
allocation with accepted distinct-RGB code, and rejects a 1.0-green fragment
that changes other machine instructions. The physical N100 passed 160 repeated
triangle draws with five colors and four shapes.
[Physical RGB evidence](../evidence/native-color-compiler-spike-021/README.md)
records the compiler matrix, exact hashes, and GPU traces.
[Physical bounded-shader evidence](../evidence/native-vulkan-green-range-017/README.md)
records the hashes and GPU trace. [Pipeline VM evidence](../evidence/native-vulkan-compiler-pipeline-006/README.md)
and [validator VM evidence](../evidence/native-intel-validator-007/README.md)
record earlier compiler checkpoints. The [recovery checkpoint](../evidence/native-ramfs-unmount-008/README.md)
records the repaired physical boot volume.
The [fence checkpoint](../evidence/native-vulkan-fence-009/README.md) records
the VM fence tests, and the
[hardware fence checkpoint](../evidence/native-vulkan-fence-hardware-018/README.md)
records 64 exact N100 draws submitted with fences.
The [submission timing checkpoint](../evidence/native-vulkan-submit-timing-019/README.md)
records 128 physical draws and opt-in host-observed ioctl and submit durations;
GPU execution time remains unisolated.
The [steady-state profile](../evidence/native-vulkan-steady-profile-020/README.md)
records 64 repeated submissions of one command buffer, fence, and BO set,
alongside three Haiku kernel profiles that point to PCI and forcewake checks as
the largest sampled CPU costs.
The [command reset checkpoint](../evidence/native-vulkan-command-reset-010/README.md)
records pool reset, and [checkpoint 011](../evidence/native-vulkan-command-reset-011/README.md)
records individual and implicit reset in the VM.
The [two-shader owner checkpoint](../evidence/native-rcs-two-shaders-013/README.md)
and [Vulkan checkpoint 014](../evidence/native-vulkan-two-shaders-014/README.md)
record the VM preflight that preceded the physical test.
