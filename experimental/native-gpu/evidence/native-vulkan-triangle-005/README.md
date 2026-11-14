# First native Vulkan N100 triangle, 2026-11-14

The [direct-linked Vulkan application](../../vulkan/triangle_probe.cpp) now
creates an instance, N100 device, the two pinned SPIR-V shader modules, a
pair of vertex buffers, a 32×32 BGRA image and framebuffer, a graphics pipeline, and a
recorded command buffer. `vkQueueSubmit` translates that one fixed draw into
the native owner version 7 request. The N100 executes the validated shader
and vertices; after owner completion and pixel verification, the userspace
driver copies the 4 KiB result into the image BO. The application invalidates
its noncoherent mapping and checks the full-image FNV hash. This is real GPU
rendering initiated through Vulkan API calls, with a synchronous CPU image
copy before readback.
The application binds vertex buffer A, records the draw, then binds B before
ending the render pass. The [red run](postdraw-rebind-red.log) rendered B and
failed the expected image check. After command recording captured the binding
at `vkCmdDraw`, the [green run](postdraw-rebind-green.log) rendered A. The
final 64-draw regression includes this state-order check in every draw.

The pipeline accepts only the exact SPIR-V vertex and fragment fixtures proven
by the pinned Mesa 26.3.0 compiler. A fragment module with the constant `.25`
changed to `.5` is rejected at pipeline creation. This path does not compile
general SPIR-V on RenkuOS. `RcsDrawGenerated.h` supplies the matching proven
Intel shader bytes and retains its Intel/MIT notice and license file.
The fixtures are SPIR-V 1.0 and validate for the driver's advertised Vulkan
1.0 API. An earlier SPIR-V 1.5 fixture was rejected by the Vulkan 1.0 validator;
reassembling the same shader sources for Vulkan 1.0 changed only each binary's
version byte. The [compiler proof](../../render/compiler/mesa-26.3.0/spirv/proof.json)
still emits byte-identical N100 VS and FS code.

The [native build](native-build.log) passed the Vulkan ABI probe and four
triangle shapes. The final [64-draw regression](final-four-shapes-64.log) ran
each shape 16 times and found these exact image hashes:

| Shape | Full-image hash |
| ---: | --- |
| 0 | `28616d2d` |
| 1 | `d40eb9c3` |
| 2 | `e24aed05` |
| 3 | `984ae945` |

The version 7 owner draw control and read-only RCS probe passed after the
64 Vulkan draws. Earlier 64-draw regressions for owner versions 1–7 are in
`../rcs-combined-015/`. The [opt-in JSON trace](trace-shape2.log) identifies
one Vulkan submission, file-local vertex, shader, and image BO handles,
owner completion sequence 873/873, 766/766 checked pixels, the shape 2 hash,
and no fault. The trace is disabled during normal operation.

The final library SHA-256 is
`8daa306f88017e5c57c0f5847ee05607e627702a99e7ad64b64858e9a1023a5e`;
the application SHA-256 is
`37c06a55f7291d5e921304687d0126cd049d727734ebe10bf2917ca508de364d`.
See [binary hashes](binaries.sha256) and [source hashes](source.sha256).
The three saved GCC depfiles and [hash manifest](manifest.json) inventory
**23 direct compile inputs totaling 1,543,166 bytes**: 1,459,702 bytes of
Khronos headers, 43,291 bytes of Vulkan driver source, 26,815 bytes of probe
sources and SPIR-V fixture, 8,610 bytes of Intel draw fixture and license,
and 4,748 bytes of native ABI headers. This is a userspace slice, not the
complete graphics stack or a model-token count. The owner/bridge direct-input
inventory is separately 668,490 bytes; the Mesa compiler closure remains
much larger and is not included here.

This experimental driver is isolated at `/boot/home/native-vulkan-triangle-005`
and is not a system loader install or Vulkan conformance claim. It has one
fixed render target, shader pair, and command shape. It lacks native general
shader compilation, asynchronous Vulkan fences/semaphores, presentation,
broader formats and pipelines, and performance measurements. No base RenkuOS
kernel source, Linux DRM/GEM interface, Zink, or OpenGL stack was added.
As with Vulkan generally, passing stale or foreign object handles is outside
the defined behavior of this experimental API and may crash the process.
