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

The next isolated CPU checkpoint adds file-local resource handle/VA policy and
offline PPGTT page binding before publication. The owner-backed buffer ioctl,
general RCS submission, fences, and ANV adapter are still to be implemented.
