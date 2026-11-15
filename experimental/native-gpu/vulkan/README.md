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
timeline semaphores, presentation, and broad Vulkan compatibility remain open. Basic
host fences now support creation, status, reset, wait, and signaling after a
successful synchronous queue submission. A physical N100 triangle probe now
tests fence state before submission, after GPU completion, and after reset.
The compiler-backed path accepts multiple submit records and command buffers
with binary semaphore dependencies; it validates the full call before executing commands in
order and signals the fence after the final successful draw. A two-submit call
and the original single-submit path passed on all four triangle shapes.
It now accepts multiple graphics pipelines in one creation call, validates
their fixed state before allocation, and rolls back earlier pipelines if a
later allocation fails. The [N100 dual-pipeline checkpoint](../evidence/native-vulkan-multipipeline-031/README.md)
records two distinct fragment programs submitted in order, resource-exhaustion
rollback, and the final pixel readback.
Command buffers can record multiple render-pass instances and bounded draws.
Submission replays their captured framebuffer, pipeline, and vertex bindings
in order and signals the fence after the last draw. The
[multi-pass checkpoint](../evidence/native-vulkan-multipass-032/README.md)
records two draws from one command buffer on the N100. The
[render-pass LOAD checkpoint](../evidence/native-vulkan-loadpass-091/README.md)
adds a bounded color attachment load across passes, using the existing
preserving owner request. Its two-pass image matched a same-pass draw pair on
the physical N100 at both supported sizes; 262 earlier regression draws passed.
The [vertex-offset checkpoint](../evidence/native-vulkan-vertex-offset-051/README.md)
records 112 physical N100 draws with `vkCmdBindVertexBuffers` byte offset,
`firstVertex`, and a second-page vertex range in an 8 KiB buffer. The owner
checks the selected 48-byte range before patching the GPU vertex address.
The [distinct vertex shader offset checkpoint](../evidence/native-vulkan-vs-offset-052/README.md)
adds a version-14 owner request for approved rotated and depth vertex shaders
with a binding offset, `firstVertex`, or a second-page vertex range. It passed
60 repeated draws on the physical N100. The fragment program for that request
remains the approved constant-color profile.
The [direct render-target checkpoint](../evidence/native-direct-target-053/README.md)
records 178 physical regression draws after adding an owner-resolved image BO
as the GPU render target. This is the default Vulkan path; set
`RENKU_VK_DIRECT_TARGET=0` to use the earlier readback-and-write path.
The [current source-closure inventory](../evidence/native-stack-current-054/README.md)
counts 14,174,197 unique canonical runtime bytes across the selected compiler,
owner, and Vulkan inputs. It tracks generated and SDK inputs separately; model
token counts and some external dependencies remain unmeasured.
The [native GL-header compiler candidate](../evidence/native-glheader-prune-055/README.md)
reduces that measured closure to 13,479,049 bytes and passed 162 physical N100
draws. It remains an isolated bundle; the installed owner and kernel were not
changed.
The [selected GL-extension constants candidate](../evidence/native-glext-prune-062/README.md)
reduces the closure further to 12,628,736 bytes. Its compiler binary is byte
identical to the 055 compiler, and the isolated 062 bundle passed 162 N100
draws. Neither candidate changes the installed owner or kernel.
The [green FragCoord checkpoint](../evidence/native-green-gradient-063/README.md)
adds a second hardware-tested gradient output channel at scales 1/32, 1/64,
and 1/128. The updated native owner and Vulkan bundle passed 178 physical N100
draws, and the selected runtime source closure is 12,629,415 bytes.
The [binary semaphore checkpoint](../evidence/native-vulkan-semaphore-064/README.md)
adds signal/wait ordering on the single synchronous queue. Its isolated Vulkan
bundle passed 28 semaphore draws and 178 rendering regression draws on the
N100. The measured selected runtime source closure is 12,633,124 bytes.
The [submission latency baseline](../evidence/native-vulkan-latency-065/README.md)
measures 768 verified N100 draws. Median blocking submissions take roughly
5.3–5.4 ms; owner-side timings are needed to locate that cost precisely.
The [owner timing experiment](../evidence/native-render-timing-066/README.md)
finds roughly 2.4 ms before command publication, 1.1 ms in the completion
wait, and 0.7–0.8 ms in post-completion checks. Its instrumented owner was
restored to the known-good installed binary after the hardware sample.
The [full-cache flush measurement](../evidence/native-render-visible-067/README.md)
found three flushes totaling roughly 1.5 ms per draw. The
[optimized owner checkpoint](../evidence/native-render-flush-068/README.md)
removes one redundant pre-publication flush and passes 974 draws with the
installed normal owner. Median blocking submit time falls about 9–11% across
the four measured 32×32/64×64 direct/fallback workloads. The selected source
closure is 12,635,680 bytes.
The [uniform-buffer Vulkan checkpoint](../evidence/native-vulkan-ubo-075/README.md)
adds a compiler-backed set-0/binding-0 fragment UBO descriptor. A Vulkan
application changed its mapped 16-byte buffer between two submissions and
verified both N100 images. The isolated compiler and Vulkan bundle also passed
222 prior hardware regression draws. The selected runtime source closure is
12,667,193 bytes, with model tokens still unmeasured.
The [64×64 UBO checkpoint](../evidence/native-vulkan-ubo64-076/README.md)
extends the same descriptor-backed fragment program to the larger render
target. Two N100 draws matched 3,476/3,476 checked pixels each, and the final
raw image was captured. The existing 222-draw regression matrix passed; the
selected runtime source closure is 12,667,163 bytes.
The [image-memory-offset checkpoint](../evidence/native-vulkan-image-offset-079/README.md)
adds page-aligned nonzero `vkBindImageMemory` offsets. Twelve combinations
passed 24 physical N100 draws, including direct and copy paths. Its selected
runtime source closure is 12,664,657 bytes.
The [N100 BLAKE3 compiler checkpoint](../evidence/native-blake3-080/README.md)
removes the unselected AVX-512 assembly from Mesa's compiler link while
retaining AVX2, SSE4.1, and SSE2. The isolated compiler and Vulkan bundle
passed 222 regression draws plus 40 offset draws on the physical N100. The
selected runtime source closure is now 12,502,676 bytes; model tokens and
broader kernel/build inputs remain unmeasured.
The [SPIRV-Tools closure checkpoint](../evidence/native-spirv-tools-closure-081/README.md)
adds the validator library's pinned source to that inventory. Its host build
selected 137 canonical project files, bringing the deduplicated selected
runtime source measurement to 15,222,168 bytes. This is a host reproduction
of the Haiku package target; the exact Haiku package build inputs, build-only
generators, and model tokens are still unmeasured.
The [current selected-source inventory](../evidence/native-stack-closure-089/README.md)
rebuilds the owner and Vulkan dependencies after the version-21 preserving
batch. Including the pinned host reproduction of SPIRV-Tools, it measures
15,241,894 unique source bytes. This supersedes the intervening delta
estimates; the complete Haiku build closure and model tokens remain unmeasured.
The [native format-header checkpoint](../evidence/native-fourcc-prune-090/README.md)
replaces the selected compiler's three Linux DRM UAPI headers with four
verified modifier constants. The physical N100 passed 262 previous regression
draws and six four-triangle image comparisons with the candidate compiler
loaded. The measured selected runtime source inventory falls to 15,072,193
bytes; the 3–5 MB target remains unmet.
With the render-pass LOAD change, the selected runtime source inventory is
15,072,397 bytes. The full build closure and model tokens remain unmeasured.
The [one-pass multidraw checkpoint](../evidence/native-multidraw-082/README.md)
preserves the render target across two draws and verifies the complete 32x32
and 64x64 images on the N100. Its initial path uses two blocking owner calls.
The [native GPU batch checkpoint](../evidence/native-gpubatch-083/README.md)
combines a bounded pair of 32x32 triangles sharing a pipeline and contiguous
vertex data into one owner call and one RCS completion. The physical N100
matched the complete output image; median blocking submit time fell 51.6%
for that workload, and the prior 262 regression draws passed.
The [64x64 native GPU batch checkpoint](../evidence/native-gpubatch64-084/README.md)
extends that versioned owner request to the four-page target. The N100 matched
all 4,096 output pixels against independently captured triangles. The median
blocking submit time was 4,893.5 µs versus 10,218 µs for two publications;
the prior 262 hardware regression draws passed again.
The [six-vertex draw checkpoint](../evidence/native-vulkan-draw6-086/README.md)
extends that path to one `vkCmdDraw` call for two triangle-list primitives.
Both target sizes matched the prior captured images byte for byte on the N100;
the existing 262 hardware regression draws passed with the candidate library.
The [multi-primitive draw checkpoint](../evidence/native-vulkan-draw9-087/README.md)
accepts additional triangle-list primitives in one `vkCmdDraw`. Nine vertices
in one call matched three established calls byte for byte at both target sizes.
It also keeps a two-triangle batch within one vertex page; a pair split across
pages falls back to two validated submissions.
The [preserving-pair checkpoint](../evidence/native-pairpreserve-088/README.md)
adds owner request version 21 for a later pair in the same render pass. Four
triangles now take two RCS submissions rather than three, with byte-identical
32×32 and 64×64 images and a 33.7% lower median blocking submission time in
the measured workload.
The [native window checkpoint](../evidence/native-vulkan-window-033/README.md)
records a compiler-backed Vulkan triangle rendered on the N100, verified by
pixel hash, and copied to a Haiku `BBitmap` for a native `BWindow` draw. Build the
one-shot app with `build-vulkan-window.sh` using `RENKU_VULKAN_HEADERS` and
`RENKU_VULKAN_LIB_DIR`; `--close-after-ms 3000` bounds its hardware run. This
readback presenter does not implement Vulkan WSI or swapchains.
The [command-buffer allocation checkpoint](../evidence/native-vulkan-cmdbatch-034/README.md)
verifies one API allocation returning two independently recorded command
buffers, then submits both on the N100. Allocation rolls back all new buffers
if host allocation fails.
The [format-query checkpoint](../evidence/native-vulkan-format-035/README.md)
adds Vulkan format and image-format queries for the currently supported linear
BGRA render target and float4 vertex format. The N100 accepted a color-only
image and produced the same triangle hash. Image creation still requires the
exact 32×32 shape; the query's 32×32 maximum cannot express that minimum.
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
noncoherent. The native interface currently supports one 256 KiB resource arena,
vertex buffers and one bounded uniform buffer, and 32×32 or 64×64 linear BGRA
color images. A synchronous
`vkQueueSubmit` submits each owner-validated hardware triangle directly into
the bound image BO. The owner returns a diagnostic pixel copy for verification.
Set `RENKU_VK_TRACE=1` for a concise
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
`vkCreateGraphicsPipelines`, requires an owner-approved vertex machine
program, and uploads a fragment program only when its machine code matches an
approved N100 profile. The constant-RGB profile has a 208-byte instruction
template, independently checked by the owner across the full zero-padded
shader page. Red and blue are bounded to `[0, 1]`; green is bounded to
`[0, 1)`. The 64×64 FragCoord-red profile accepts a three-scale family of
352-byte programs for `gl_FragCoord.x / 32`, `/ 64`, or `/ 128`, using Vulkan
pixel-center coordinates. The owner verifies each completed image against a
CPU reference. [Initial FragCoord evidence](../evidence/native-fragcoord-vulkan-047/README.md)
records the pixel-center correction; [scale-family evidence](../evidence/native-gradient-family-049/README.md)
records 72 physical submissions and full image captures. Other machine code
and pipeline states are rejected.
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
