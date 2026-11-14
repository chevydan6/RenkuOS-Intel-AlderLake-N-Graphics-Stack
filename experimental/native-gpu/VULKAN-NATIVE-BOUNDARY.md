# Mesa 26.3.0 ANV and native N100 boundary

Date: 2026-11-14. Scope: Vulkan only. This is a host build and physical RCS
baseline, not a native ANV execution result. The current kernel driver and
installed binaries were not changed.

## Verified starting point

The physical Haiku/RenkuOS N100 reports `8086:46d1`, revision 0. A read-only
RCS probe passed. The installed native fixed draw then passed 64 consecutive
shader draws (16 rounds of four triangle variants). Every result had the
expected image hash, 654/654 checked pixels, matching nonce and completion
sequence, and a gap-free sequence ending at 2746. The repeatable runner is
`render/rcs_hardware_regression.py`; raw results and
metadata are in `evidence/native-rcs-vulkan-baseline-006/`.

## ANV host build and measured inputs

The pinned Mesa 26.3.0 source archive SHA-256 is
`91d729aa68bb89e4ec857841300bbe922bcb3e52b1f97566ca83930d3b28343a`.
An isolated ANV-only Meson configuration selected `-Dvulkan-drivers=intel`,
`-Dgallium-drivers=`, `-Dopengl=false`, `-Dplatforms=`, `-Dintel-rt=disabled`.
LLVM 18 and glslang 15 were needed by this configuration's build tools. The
host `libvulkan_intel.so` built successfully in 1,047 Ninja steps. It is a
Linux/libdrm host artifact, not a native Vulkan driver.

`evidence/anv-host-build-001/measure-target-compile-inputs.py` selected the 858
compiled objects in that target's Ninja command graph and their recorded
compiler depfiles. The hash-indexed
`evidence/anv-host-build-001/anv-compile-inputs.json` (SHA-256
`4454e6ba9348882863c9998bb67a1c5e269e89fb4e549597df33dd45eee01135`)
contains:

| Direct compile input category | Files | Bytes |
| --- | ---: | ---: |
| Mesa canonical source and headers | 1,144 | 25,905,630 |
| Generated build inputs | 128 | 188,249,320 |
| External headers | 978 | 18,152,893 |

The selected build compiles multiple Intel generations, generated tables and
internal shaders. These figures are not a minimized N100 source closure or model
token count. They do show that an unmodified ANV build does not meet a 3–5 MB
project-source target. Disabling other generations and features needs correctness
and hardware tests before any size claim.

## Exact native platform gap

ANV's `anv_kmd_backend` interface (`src/intel/vulkan/anv_kmd_backend.h`)
requires BO creation, mapping and close, GPU VA binding, queue execution and
sync dependencies. The Linux i915 implementation calls GEM/DRM ioctls from
`src/intel/vulkan/i915/anv_kmd_backend.c` and queue/context code. Shader
compilation in `anv_shader_compile.c` uses NIR and `brw_compile`; those are
valuable reusable logic independent of the Linux kernel interface.

The installed native RCS v1 ioctl accepts only PROBE, BOOTSTRAP and one of four
kernel-built DRAW variants. Its eight fixed 4 KiB draw pages map at GPU
`0x400000` through `0x407000`; the caller cannot create BOs, submit command
words, bind GPU addresses or receive general fences. It returns a synchronous,
validated 32×32 image. Consequently an ANV backend cannot yet execute even a
minimal general Vulkan command buffer through this ABI. The next kernel/RCS
increment must establish owned GPU resources and a bounded submission contract
with completion, visibility and failure semantics. Preserve the fixed draw as
the hardware regression control while adding those capabilities.

No Vulkan triangle, conformance, native presentation, or actual model-token
measurement is claimed here.

## Later RCS progress on 2026-11-14

The v2 owner is now installed on the physical N100. A 64-draw regression
accepted user-supplied triangle vertices, verified four distinct pixel hashes,
and preserved the v1 64-draw control; see
`evidence/rcs-user-vertices-002/`. This remains a kernel-selected batch and
shader path, so the ANV BO/bind/submit gap above remains open.

The next checkpoint adds file-local resource handle/VA policy, prepublication
PPGTT binding, and one physical owner-backed vertex page. Version 3 rendered
64 N100 triangles from that page with pixel hashes identical to v2. The v1 and
v2 64-draw controls also passed; see `evidence/rcs-resource-vertices-001/`.
At that checkpoint, the owner-backed buffer ioctl, dynamic allocation and
binding, general RCS submission, fences, and ANV adapter remained unfinished.

The next native increment installed a 16-page prebound resource pool and a
file-local create/read/write/destroy ioctl. Sixty-four resource sessions and
all three 64-draw RCS controls passed on physical N100 hardware; see
`evidence/rcs-resource-ioctl-001/`. Public buffer objects are not yet used by
an RCS draw. The next vertical slice must patch a validated vertex-buffer
address from one such object and prove pixels and completion. The 16-page
pool avoids post-publication PTE changes but is not a general Vulkan VM.

Version 4 now consumes a buffer created and filled through the public native
ioctl. On physical N100 hardware, 64 public-buffer draws passed with four
expected pixel hashes and gap-free completion, and 64-draw v1/v2/v3 controls
remained green; see `evidence/rcs-public-buffer-001/`. The private fixed vertex
page is zeroed in v4, establishing that the GPU read the public BO. This
closes the first create/write/address/submit/readback vertical slice, but the
submission is still a kernel-selected RCS batch. General Vulkan commands,
fences, shader compilation, and presentation remain unimplemented.

Version 5 now submits a public command BO through the same native owner. The
owner verifies every 4 KiB batch dword against a known-safe RCS draw image,
allows only an owner-resolved vertex BO address, and points MI_BATCH_BUFFER_START
at the public BO. Sixty-four N100 hardware draws and all v1–v4 64-draw
controls passed; see `evidence/rcs-public-batch-001/`. This proves a bounded
userspace command-buffer path. It does not satisfy ANV's general command
submission, sync, or VM-binding contract.

The installed v5 owner/bridge compile inputs are now hash-inventoried from
four GCC depfiles. The 140 recorded files total 663,470 bytes, including
311,805 bytes of Haiku SDK headers and a generated owner translation unit
that duplicates canonical owner content. The scope and complete manifest are
in `evidence/native-closure-001/`. This is the v5 native RCS input baseline,
not a Vulkan closure or model-token count. Mesa ANV's selected host compile
inputs above remain a separate, much larger measurement.

An isolated Mesa 26.3.0 Intel compiler harness builds and deterministically
emits VS/FS machine code from direct NIR. Its five artifacts and corruption
checks are recorded in `render/compiler/mesa-26.3.0/`. The regenerated command
image passed 64 hardware draws in each of v1–v5, plus four final v5 draws;
see `evidence/rcs-mesa263-shader-001/`. The installed native driver still has
no Vulkan SPIR-V ingestion or RenkuOS userspace compiler.

A focused Mesa 26.3.0 host proof now ingests two validated Vulkan 1.2 SPIR-V
modules, converts them to NIR, and emits VS/FS code byte-identical to the
hardware-tested direct-NIR image; see `render/compiler/mesa-26.3.0/spirv/`.
This establishes the first shader-compiler vertical slice. It does not yet
provide a native RenkuOS compiler service or general Vulkan pipeline support.

Render command version 6 now takes the Mesa 26.3.0 fragment shader from a
file-local public BO. The owner checks the entire 4 KiB page against the
proven shader, points the PS SIMD kernels to the owner-selected address, and
zeroes the private FS page. On physical N100 hardware, 64 v6 draws and all
five previous 64-draw controls passed; see `evidence/rcs-public-shader-001/`.
This closes one shader upload/address/execute/readback path. The ABI still
admits only exact known-safe code and fixed graphics state; it is not yet a
Vulkan pipeline or general GPU shader submission interface.

The installed v6 owner/bridge input inventory is now 141 files and 666,930
bytes, up 3,460 bytes from v5; see `evidence/native-closure-v6-002/` for
depfiles, per-file hashes, scope, and exclusions. It remains far narrower than
the complete Vulkan stack and is not a model-token count.

A direct-linked native `libvulkan.so` now compiles on RenkuOS with the pinned
Khronos headers and enumerates the physical N100 through the existing probe
ioctl. It also creates one graphics device/queue and owns shader-module
objects for the two validated SPIR-V fixtures; see `vulkan/` and
`evidence/native-vulkan-abi-001/`. This is a genuine native Vulkan ABI entry
point, but it has not submitted a Vulkan draw or reached conformance. The
remaining userspace gap is graphics-pipeline, memory, command-buffer, queue
submission, readback, and presentation semantics over the native owner.

The next native Vulkan slice now creates owner-backed `VkDeviceMemory` and a
vertex `VkBuffer`, binds them, and uses explicit mapped-memory flush and
invalidate to verify exact triangle-vertex bytes through the owner resource
ioctl. Device creation bootstraps the owner even after a clean reboot.
The N100 probe passed once after reboot, 64 repeated memory sessions, and 64
more after a requested-size bounds fix; the v6 shader hardware draw passed
after each series. See `evidence/native-vulkan-memory-002/`. The advertised
memory type is host-visible and noncoherent, matching the explicit transfer
path. The next userspace blocker remains graphics-pipeline and command-buffer
translation into the bounded RCS draw contract. Vulkan has not submitted a
draw, and the current ABI does not offer general command submission.

The fixed 32×32 BGRA Vulkan image object now accepts a 4 KiB owner BO as its
memory. Image creation rejects unsupported dimensions. Sixteen physical image
object sessions and the v6 hardware draw passed; see
`evidence/native-vulkan-image-003/`. The image does not yet receive rendered
pixels through Vulkan; render-pass, pipeline, command, queue, and readback
semantics remain to be implemented.
