# Initial technical report — before implementation

## Finding and first achievable milestone

Haiku master at `4f1074d5e14f59e0f5e5c4938c8a50d1290fa59c` already maps
`46d1` to `INTEL_MODEL_ALDM` in `intel_extreme/driver.cpp`. Adding that ID
again would accomplish nothing. `intel_gart.cpp` separately matches an Intel
bridge and display pair. It lists `4621:46d1`, but the verified N100 has
`461c:46d1`. `intel_init()` checks both, and `intel_extreme_init()` requires
`gGART->map_aperture()` to succeed. The missing pair is therefore an attachment
blocker on this hardware in the inspected source. This is source-level evidence;
physical Haiku attachment has not yet been observed.

First proposed patch: add **only** `461c:46d1` with the existing ALDM device
classification; preserve the prior pair. Compile the real module and test table
matching against known, mismatched and absent devices. Do not add unverified
46d0/46d2 variants or pretend the ID addition validates initialization.

A second narrow finding: `hooks.cpp` advertises blit/fill/invert functions
unconditionally, although `intel_wait_engine_idle()` explicitly skips LAKE and
SOC0 families because their engine is unsupported. The functions still enqueue
legacy commands. Suppress those three hooks for the already excluded families.
This is a capability-safety fix, not acceleration. Keep display hooks intact.

## Existing native functionality

- `src/add-ons/kernel/drivers/graphics/intel_extreme`: PCI discovery, MMIO/shared
  areas, framebuffer/aperture allocation IOCTLs, BIOS/VBT parsing, power helpers,
  display interrupts including Gen11/Gen12 paths. Driver opens have side effects.
- `src/add-ons/kernel/busses/agp_gart/intel_gart.cpp`: paired PCI discovery,
  stolen-memory sizing, aperture mapping, scratch pages, GGTT updates including
  64-bit entries for generation >=8. This is not a GEM/per-process GPU VM system.
- `src/add-ons/accelerants/intel_extreme`: display modes, ports, pipes, PLLs,
  EDID/AUX, framebuffer configuration, DPMS, cursor, and legacy ring/2D commands.
  ALDM reports generation 12. Existing display support is worth preserving;
  laptop eDP and NanoKVM HDMI operation are not yet verified under Haiku.
- `headers/os/add-ons/graphics/Accelerant.h`: C accelerant hook ABI.
  `headers/private/graphics/intel_extreme/intel_extreme.h`: shared state/IOCTLs.
- `src/servers/app/drawing/interface/local/AccelerantHWInterface.cpp`: mode,
  framebuffer and cursor integration. The inspected local implementation does
  not acquire legacy 2D hooks; virtual DWindow code does. A blitter hook by itself
  therefore does not establish accelerated desktop compositing.
- `engine.cpp`: legacy primary ring submission and polling; token functions do
  not implement modern completion fences. Ring-space timeout currently breaks
  then subtracts space, a separate safety issue to audit before any execution.
  Ring setup is still unconditional; hook suppression alone does not remove
  all legacy MMIO initialization or establish safe Gen12 execution.

## Reusable implementations and gaps

Mesa 26.0.7's `src/gallium/drivers/iris` supplies modern Intel state emission,
buffer handling, shaders via `src/intel/compiler`, and surfaces via
`src/intel/isl`. Keep these implementations in C. `i915/iris_kmd_backend.c`
requires allocation, mapping/cache policy, contexts, reset status and execbuffer2;
`iris_bufmgr.c` also needs DRM sync objects, handle lifetime and external sharing.
`src/intel/common/i915/intel_gem.c` creates contexts and queries kernel features.
Mesa's Haiku build defaults to llvmpipe/softpipe, and Haiku is absent from the
KMS/DRM platform list in this release. Existing hgl/Haiku presentation glue is
reusable, but is not a hardware Iris backend.

ANV (`src/intel/vulkan`) shares compiler/device/surface infrastructure and adds
Vulkan memory, queue, timeline and presentation requirements. It comes after
working buffer ownership, submission and recovery; no Vulkan version is promised.

Linux i915 is the primary reference for this GPU: display, GGTT/PPGTT, logical
contexts, engine reset, forcewake, power and firmware ordering. Xe is useful as
a newer VM-bind/exec/synchronization design reference, but is not the active
host driver and is not a small substitute for Haiku's architecture. Reusing
hardware algorithms is feasible; importing Linux scheduling, GEM/TTM, dma-buf,
fences and memory internals wholesale is not the first milestone.

## Licensing and references

Haiku files inspected use MIT notices. Mesa Iris/Intel files inspected carry
permissive notices; review each imported file and generator dependency and retain
notices. Linux has mixed per-file licenses, including GPL-only components and
some permissive Intel files: use it as a reference and do a per-file SPDX audit
before copying. UAPI licensing does not grant permission to copy arbitrary kernel
implementation into Haiku. Firmware licenses are separate. No Linux/Mesa code
or firmware is incorporated by the initial patches.

Primary references (source snapshots are pinned above):

- [Haiku driver](https://github.com/haiku/haiku/blob/4f1074d5e14f59e0f5e5c4938c8a50d1290fa59c/src/add-ons/kernel/drivers/graphics/intel_extreme/driver.cpp)
- [Haiku GART](https://github.com/haiku/haiku/blob/4f1074d5e14f59e0f5e5c4938c8a50d1290fa59c/src/add-ons/kernel/busses/agp_gart/intel_gart.cpp)
- [Haiku engine](https://github.com/haiku/haiku/blob/4f1074d5e14f59e0f5e5c4938c8a50d1290fa59c/src/add-ons/accelerants/intel_extreme/engine.cpp)
- [Mesa Iris backend](https://gitlab.freedesktop.org/mesa/mesa/-/blob/mesa-26.0.7/src/gallium/drivers/iris/i915/iris_kmd_backend.c)
- [Mesa Haiku build selection](https://gitlab.freedesktop.org/mesa/mesa/-/blob/mesa-26.0.7/meson.build)
- [Linux i915 documentation](https://www.kernel.org/doc/html/latest/gpu/i915.html)
- [Intel supported i915 devices](https://dgpu-docs.intel.com/overview/supported-hardware/i915-driver-gpus.html)
- [Intel Xe-LP guide](https://www.intel.com/content/www/us/en/developer/articles/guide/lp-api-developer-optimization-guide.html)

Detailed PRM register/command auditing remains required before programming a
Gen12 engine. Neither API optimization guides nor device-ID tables suffice to
validate GPU commands.
