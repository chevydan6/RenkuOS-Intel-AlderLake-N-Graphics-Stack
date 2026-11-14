# Native N100 Vulkan stack source inventory

This inventory combines the pinned Mesa 26.3.0 Intel/SPIR-V compiler
link/include file list from `native-runtime-closure-022` with fresh GCC
dependency files from the Haiku VM owner/bridge and Vulkan library builds.
All 878 compiler files were present and rehashed on the VM. The owner and
Vulkan manifests hash the current direct build inputs. `installed-hashes.log`
ties their binaries and the Mesa-derived compiler library to the physical
N100 checkpoint that passed the 64×64 vertex-shader tests.

| Scope | Files | Bytes |
|---|---:|---:|
| Mesa source selected by compiler link/include list | 533 | 14,025,279 |
| Native owner bundle inputs | 49 | 231,930 |
| Original Haiku owner source/headers | 15 | 100,367 |
| Native Vulkan implementation source | 1 | 56,792 |
| Khronos Vulkan headers | 15 | 1,459,702 |
| Generated compiler build files | 49 | 62,392,205 |
| Haiku compiler and owner SDK inputs | 372 | 5,302,141 |

Across all categories, the path union is 1,058 files and 83,750,618 bytes.
Identical file contents counted once total 82,280,782 bytes. The selected
canonical runtime inputs, excluding generated files, SDK headers, and test
probes, total **14,490,381 unique bytes**. This includes whole source/header
files even when only a small portion is used. The Khronos headers are mostly
identical to copies already in the Mesa source tree, so simply summing rows
would double-count them. The generated owner translation unit also duplicates
canonical owner logic and is kept in its own category.

This is a build-input inventory and a lower bound for the complete source
closure, not a minimum port size. The compiler file list was selected by the
prior 352-object link measurement and rehashed here; this run did not repeat
link-object selection. SPIRV-Tools implementation source, build-only generator
source, and RenkuOS kernel source outside the owner closure are still missing.
The 3–5 MB source target is therefore not currently met. Binary size and the
62 MB of generated files are separate measurements. No development-model
tokenizer is available in this workspace, so model-token counts remain
unmeasured rather than inferred from bytes.

The selected compiler objects include Mesa's i915 device-info object, which
pulls in the 131,737-byte `include/drm-uapi/i915_drm.h` header. Large GL/GLES
headers also appear through Mesa's shared compiler dependencies. The staged
compiler library links `libSPIRV-Tools-shared.so`, C++ runtime libraries, and
Haiku `libroot.so`; it does not link `libdrm`. The i915 device-info object is a
specific source-closure reduction candidate, but this inventory does not
prove that removing it preserves required N100 device initialization.

Reproduce the combined manifest with `measure-stack-closure.py` using the
three input paths and VM host recorded in `stack-manifest.json`.
