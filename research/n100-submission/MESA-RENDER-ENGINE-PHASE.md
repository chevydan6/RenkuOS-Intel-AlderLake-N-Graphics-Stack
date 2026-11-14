<!-- SPDX-License-Identifier: MIT -->
# Separate render-engine phase and Mesa source-tree guide

2026-11-14; native branch baseline `f1505b8`.

## Scope and decision

**Shaders, OpenGL and Vulkan are a separate render-engine phase.** The completed
BCS milestones prove native copy-engine execution, owned-buffer copies and
request retirement. The desktop investigation selected a possible batched
presentation boundary. None of those establishes render-engine initialization,
EU shader execution, hardware OpenGL, hardware Vulkan or a working Mesa port.

Keep the validated native BCS path and its Intel hardware logic. Reuse its memory
ownership, DMA admission, address-space and retirement foundations where their
contracts apply. Add render-specific hardware support separately; do not change
a BCS descriptor's engine number and assume the rest is a render context.
Do not resume full Linux i915/LinuxKPI source closure as the prerequisite for
native rendering.

Iris and the Haiku frontend are source references and potential reuse targets,
not drop-in integrations. The [official Mesa source-tree guide][tree] is a useful
orientation. Exact files below are checked against release archives, not assumed
from the moving documentation or another Mesa version.

Reference baselines:

- **Mesa 26.0.7**, main Iris/compiler/Haiku EGL/ANV reference. Its official
  [release checksum][release26] matches the downloaded archive:
  `0c56bbcf1947e1a6a90ac09b129b0ca0cb52cc31145b94595e57c8804cf02496`.
- **Mesa 22.0.5**, historical classic Haiku `BGLView`/renderer-addon reference,
  not a proposal to use its old Iris internals. Its official
  [release checksum][release22] matches the existing archive:
  `5ee2dc06eff19e19b2867f12eb0db0905c9691c07974f6253f2f1443df4c7a35`.

The [source manifest](render-phase-005/source-manifest.json) records archive URLs,
upstream release-tag links, 26 inspected files and their SHA256 hashes. Neither release is claimed to be the installed
RenkuOS Mesa package. Distro patches, package/loader ABI and the actual active
renderer must be checked when selecting a frontend build. Do not mix internal
Gallium/NIR interfaces from these versions.

## What carries forward, and what does not

| Existing foundation | Reuse in render work | New work still required |
|---|---|---|
| Native PCI/power/owner checks and observed VT-d-disabled DMA admission | Same truthful device/ownership/admission policy. | Render-engine power, reset, ownership and workarounds must be established. General IOMMU mapping remains separate. |
| Pinned pages, PPGTT construction, owned mappings | Allocation/binding/lifetime principles and validated helpers. | General-sized BOs, GPU VA management, resource metadata, CPU mappings and mapping generations. The current copy endpoint has two fixed 4 KiB data pages. |
| BCS context, HWSP, CSB and request sequences | Submission ledger, bounded polling and fail-stop retirement pattern. | RCS-specific context image/registers, descriptor/status interpretation and state initialization. Existing helpers enforce BCS engine/profile assumptions. |
| Ordered completion plus CPU/GPU visibility transitions | Ownership must remain held until all GPU access ends. | Render/texture caches, `PIPE_CONTROL`, render-target writes, instruction/state visibility and cross-engine ordering. CPU `wbinvd` is not a replacement for all GPU cache transitions. |
| Kernel-built non-overlapping linear 32-bit copies | Upload/readback/presentation when layouts are compatible. | Shader code/state, surface layout, descriptors/binding tables, samplers, vertex input and draw/dispatch commands. Tiled/compressed resources need suitable resolve/layout handling. |
| Resident backing retained on uncertain completion | Keep affected resources/context alive and stop unsafe reuse. | Multiple contexts/BOs, dependency fences, reset/lost-context reporting, client teardown and eventual reclamation. |

A future dependency chain may be CPU upload → BCS copy → render request →
BCS readback/presentation. It needs real producer/consumer dependencies and cache
visibility between engines, not only each engine's local sequence counter.
For the first proof, serialization and polling can remain; they are not a
complete Gallium fence or Vulkan synchronization implementation.

## Mesa source-tree reading map

Paths in this table are present in the checked **26.0.7** archive. The generic
Mesa directory guide does not guarantee that a particular platform backend works.

| Read | Paths | Purpose for this project |
|---|---|---|
| API and state tracker | `src/mesa/main/`, `src/mesa/state_tracker/`, `src/mesa/glapi/` | OpenGL API/state/dispatch to Gallium. This is above the kernel submission interface. |
| Gallium interfaces | `src/gallium/include/pipe/p_screen.h`, `p_context.h` | Device/resource capabilities and rendering/context operations Iris must implement. |
| Shader IR | `src/compiler/glsl/`, `src/compiler/nir/`, `src/compiler/spirv/` | GLSL/SPIR-V ingestion and NIR processing; not GPU execution by themselves. |
| Intel compiler | `src/intel/compiler/`, particularly `brw/` | Device-specific lowering, code generation and shader metadata. In this release `iris_program.c` calls `brw_compile_vs/fs/cs` and related stages. |
| Intel device description | `src/intel/dev/` | Device, fused topology, engine and capability information. A PCI ID alone is not a truthful complete device query. |
| Surface/packet support | `src/intel/isl/`, `src/intel/genxml/`, `src/intel/blorp/` | Surface layouts/formats, packet descriptions/packing, and blit/resolve support. BLORP is not synonymous with this prototype's BCS linear-copy packet. |
| Iris screen and contexts | `src/gallium/drivers/iris/iris_screen.c`, `iris_context.c`, `iris_batch.c` | Startup requirements and separate render/compute/blitter batch management. |
| Iris programs and hardware state | `iris_program.c`, `iris_program_cache.c`, `iris_state.c`, `iris_draw.c`, `iris_pipe_control.c` | Shader compilation/upload, state programming, drawing and pipeline/cache ordering. |
| Iris resources and completion | `iris_resource.c`, `iris_bufmgr.c`, `iris_fence.c`, `iris_fine_fence.c` | Resources, BO lifetime/dependencies, CPU waits and Gallium fences. |
| Iris KMD seams | `iris_kmd_backend.h`, `iris_kmd_backend.c`, `i915/`, `xe/` | OS/kernel buffer/submission backends plus context/batch setup. Not a ready-made RenkuOS backend. |
| Shared Intel OS dependencies | `src/intel/common/`, `src/intel/common/i915/` | Helpers called outside the Iris backend vector; audit these too. |
| Haiku OpenGL frontend | `src/gallium/frontends/hgl/` | Gallium/state-tracker drawable and context integration. |
| Haiku platform/presentation reference | `src/egl/drivers/haiku/egl_haiku.cpp`, `src/gallium/winsys/sw/hgl/` | The checked native EGL path and software `BBitmap` display targets. |
| Intel Vulkan | `src/intel/vulkan/` | ANV, a separate Vulkan driver; not Iris with Vulkan enabled. |
| Vulkan runtime/presentation | `src/vulkan/runtime/`, `src/vulkan/wsi/` | Vulkan object/runtime support and platform swapchain/presentation integration. |

Do not confuse `src/gallium/drivers/i915/` (the older Intel Gallium driver) with
`src/gallium/drivers/iris/i915/` (Iris's Linux i915 KMD backend). Nor should one
invent a single Iris winsys directory as the whole port: important OS dependencies
are embedded in Iris and shared Intel code.

[ISL's documentation][isl] is useful for the surface/layout boundary: rendering
resources carry format, tiling, alignment, pitches and possibly auxiliary state.
A raw linear 32-bit copy cannot be assumed to preserve every image layout.

## Concrete Iris seams to audit before attempting a native adapter

The checked source identifies these actual requirements, not just build errors:

- `iris_screen_create()` obtains a buffer manager from an fd, consumes its
  `intel_device_info` and explicitly requires context isolation. Its allocations
  include workaround/identifier resources before normal drawing starts.
- `iris_context.c` initializes **render, compute and copy** contexts through
  different vtable methods. `iris_batch.c` selects i915 or Xe initialization.
  The i915 batch code queries engines and constructs hardware contexts. Compute
  work can select the render engine when no dedicated compute engine is exposed;
  do not invent a separate physical CCS engine for N100.
- `iris_kmd_backend.h` covers create/close, mmap, caching/madvise, reset checks,
  batch submit and VM bind/unbind. **It is not the whole OS interface.** Context
  creation/parameters, device/engine queries, synchronization and common Intel/
  libdrm helpers occur elsewhere. The checked Iris Meson target depends on libdrm.
- i915 `batch_submit` builds a validation list of BO handles, GPU addresses and
  read/write use, sets pinned/no-relocation submission flags, supplies a context
  ID and may attach sync-object fence arrays. A fixed copy ioctl cannot satisfy
  these semantics.
- i915 `gem_vm_bind()`/unbind are no-ops **because submission's execbuffer list
  performs binding/validation in that backend**. Copying these no-ops into a
  native backend without implementing equivalent residency/binding is incorrect.
- Iris BO and fence code tracks read/write dependencies and makes DRM syncobj
  calls outside a simplistic submit wrapper. Real completion, waits, errors and
  context loss are required; returning success for unsupported synchronization
  would break resource lifetime.

The native adaptation direction is to preserve Mesa/Intel compiler, layout and
hardware state/command logic and implement the missing OS contracts against a
native buffer/context/submission/synchronization interface. A new backend plus
an audit of shared callers is a candidate design, not a guarantee that changing
one file removes DRM. Do not rebuild an entire Linux kernel compatibility layer
merely to make unchanged Iris/i915 translation units compile.

Moving from the current fixed kernel-built workload to Mesa-generated batches
also changes the trust boundary. Native submission needs owned handles, context/
VA isolation, valid residency and access ranges, bounded batches and real
completion/error semantics. Do not expose MMIO or writable GPU page tables as
a substitute for those contracts. This guide does not select a complete ABI.

## Haiku frontend: useful code, not automatic Iris selection

In checked **26.0.7**:

- `src/egl/drivers/haiku/egl_haiku.cpp` creates `hgl_create_sw_winsys()` and then
  `sw_screen_create()`. It does not create an Iris screen.
- `frontends/hgl/hgl.c` manages drawable textures and front-buffer flush callbacks.
  That gives a frontend/resource/presentation reference, not a render-engine
  implementation.
- `winsys/sw/hgl/hgl_sw_winsys.cpp` creates CPU/software display targets using
  `BBitmap` or CPU data, maps CPU bits, and hands a bitmap to `BitmapHook` for
  display. Its displaytarget import returns NULL and export returns false.
  Those stubs are not working GPU-buffer sharing.
- Swapping a software screen constructor for `iris_screen_create()` alone would
  leave device discovery, resource mapping, fences, image layout and native
  presentation unresolved.

For classic **22.0.5**, the checked reference paths are `src/hgl/GLView.cpp`,
`GLRenderer.cpp`, `GLRendererRoster.cpp` and
`src/gallium/targets/haiku-softpipe/{SoftwareRenderer,GalliumContext}.cpp`.
They illustrate `BGLView`/renderer add-on loading, context binding and swapping;
`GalliumContext` explicitly creates a software screen and `SoftwareRenderer`
presents a `BBitmap` through `DrawBitmap` or direct CPU copying. These classic
`src/hgl` and `haiku-softpipe` target paths are **absent from the 26.0.7 archive**.
Treat their public frontend and internal Gallium interfaces as versioned code,
not a recipe to link an old renderer target against arbitrary new Mesa internals.

A first hardware OpenGL frontend can use a completed offscreen render resource
and a CPU readback/bitmap bridge for proof, if clearly measured and described as
such. That is not zero-copy presentation. Later integrate the owned-buffer and
presentation contract identified in the
[desktop investigation](DESKTOP-INTEGRATION-INVESTIGATION.md), with fences,
resize/lifetime handling and explicit format/layout conversion.

## Vulkan is another integration, not a consequence of Iris

The relevant driver is **ANV** in `src/intel/vulkan/`, sharing Intel compiler,
layout and other support with the rest of Mesa. Its checked device code also
uses DRM/i915/Xe-specific infrastructure. Vulkan additionally needs its own
resource/memory model, pipelines/descriptors, command buffers, queues, barriers,
fences/semaphores and truthful capability reporting; native runtime/ICD loading
and platform WSI/presentation must be addressed separately.

The checked 26.0.7 `src/vulkan/wsi/meson.build` selects several platform sources
and headless support, but no Haiku platform source. That is a statement about
this release's WSI build, not proof that no external Haiku port exists. No
standard or private Haiku surface ABI is invented here. Headless/offscreen ANV
work can precede a native window/swapchain implementation; a Haiku OpenGL
frontend does not provide that Vulkan WSI contract. See also [ANV's binding-model
documentation][anv] for hardware descriptor/state complexity beyond copying.

Software llvmpipe/lavapipe results, an OpenGL version string, or running a GL app
through software do not establish Intel GPU execution. Zink's use of Vulkan
does not remove the need for a working hardware Vulkan backend. Report actual
driver identity and supported features; do not advertise Linux Iris/ANV's API
versions before the native implementation supports them.

## Bounded milestones for the separate phase

1. **Native render-engine foundation:** establish actual engine/topology data,
   an owned render context/address space, engine-specific state/cache ordering,
   bounded submission, completion and failure retention. Continue using private
   memory; do not take over scanout as a prerequisite.
2. **Offscreen shader execution proof:** reuse the Intel compiler for a small,
   known shader workload, retain the compiled code/metadata and submit it with
   the required render state. A simple vertex/fragment-shader draw into an owned
   uncompressed target with validated readback is a useful first render proof.
   Compilation alone, a clear, or a BCS copy is not proof of shader execution.
3. **Native buffer/context/fence contract and Iris integration:** supply the
   resource sizes, GPU VAs, mapping/cache rules, residency and dependencies that
   the selected Iris path actually uses. First demonstrate one offscreen context
   without fabricating unsupported capabilities; broaden from observed needs.
4. **Native OpenGL frontend and presentation:** choose a coherent Mesa/frontend/
   loader version, validate context/swap/resize behavior, and connect the render
   target to a defined readback or owned presentation path. Identify and measure
   any CPU fallback/copying. Claim API support only from implemented capabilities
   and appropriate correctness/conformance evidence, not a package name.
5. **ANV/Vulkan phase:** reuse the completed native contracts and Intel support
   where suitable, then implement Vulkan-specific semantics and native WSI.
   Hardware OpenGL does not complete this milestone.

The measured BCS costs remain a warning about tiny synchronous staging and
whole-machine cache flushes, not a bound on future shader throughput. Reuse BCS
where transfer work benefits, with real render-to-copy dependencies. Desktop
presentation can progress independently; a general render stack is not required
merely to investigate batched presentation.

## Verification and status

This work downloaded and hash-verified the official Mesa 26.0.7 release archive,
verified the existing 22.0.5 archive against its official checksum, inspected the
listed source files and checked the source-tree paths/version differences.
Independent review verified both archive checksums, all 26 file hashes and the
material source/phase-boundary findings, with no material issues found.
No Mesa build, shader compilation, render-context submission, OpenGL/Vulkan
hardware run, package installation or driver change was performed or claimed.
Installed native BCS binaries and desktop behavior remain unchanged.

[tree]: https://docs.mesa3d.org/sourcetree.html
[release26]: https://docs.mesa3d.org/relnotes/26.0.7.html
[release22]: https://docs.mesa3d.org/relnotes/22.0.5.html
[isl]: https://docs.mesa3d.org/isl/index.html
[anv]: https://docs.mesa3d.org/drivers/anv.html
