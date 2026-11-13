<!-- SPDX-License-Identifier: MIT -->
# Iris interface feasibility — source-level audit

Reference: Mesa 26.0.7 (`27cdb9ff9315a904b9827f9e711e68bb5e856e50`),
`src/gallium/drivers/iris` and `src/intel/common/i915`. This is a requirements
inventory, not an implemented API or claim that an unchanged Iris build runs.

| Requirement seen in source | Existing native Intel driver | Required work |
| --- | --- | --- |
| Device/engine/topology/capability queries | Device type/PCI table, display data | Return actual fused topology, engine class and supported features; reject unsupported capabilities |
| GEM_CREATE/CREATE_EXT and GEM_CLOSE | Aperture allocations, not per-open GEM objects | Kernel-owned handles, quota, checked size, cleanup after completion, allocation failures |
| GEM_MMAP_OFFSET/MMAP, SET_DOMAIN, SET_CACHING | Shared framebuffer/aperture areas | Checked CPU mapping lifetime, cache policy and CPU↔GPU transitions |
| Context creation/parameters/destruction | Legacy shared primary ring | Gen12 logical contexts and per-context ownership; never claim engine maps without separate state |
| Per-context GPU addresses | GGTT mappings | PPGTT allocation, binding validation, page-table coherency, teardown after DMA stops |
| execbuffer2 pinned 48-bit object addresses, NO_RELOC, HANDLE_LUT, BATCH_FIRST | Legacy direct commands | Validate handles, offsets, access and batch bounds; truthful rejection of unsupported flags |
| Exec fence arrays and sync-object create/destroy/wait/signal | Polling and incomplete legacy tokens | Real completion/error state, ordered waits, wakeup and timeout handling |
| Reset statistics / lost contexts | No complete Gen12 recovery | Detect hangs, bounded reset, fail outstanding work and invalidate lost state |
| PRIME handles, dma-buf/sync-file import/export | No equivalent in inspected path | Later sharing/presentation design; disable dependent API extensions until implemented |
| GEM_USERPTR / memory advice / protected contexts | Not present | Decide optional features from actual startup/use paths; reject rather than fabricate support |

`i915_batch_submit()` supplies a validation list, fixed GPU addresses and context
ID; it may add sync-object fence arrays. The i915 backend's VM-bind methods are
no-ops **because actual binding occurs at execbuffer submission**. Copying only
the no-op methods would omit essential memory validation and binding.

Iris calls both its KMD backend and common Intel/libdrm helpers. A new backend
alone does not remove every DRM dependency. Start with a small offscreen test,
then audit screen initialization, context setup, fences, buffer manager, device
queries and Haiku hgl presentation as a complete path. External sharing and
Vulkan timeline/presentation work can wait only if the corresponding capabilities
are accurately disabled. OpenGL versions must follow conformance and hardware
results, not Linux's GL version string.

Reuse Mesa compiler/NIR, ISL and command/state generation in their existing C
implementation. Keep new RenkuOS buffer/context/kernel glue in C++ with a
versioned C ABI. Do not expose writable page tables or unrestricted MMIO to an
untrusted client just to satisfy a call.

## Linux sources reviewed for future native implementation

The following Linux v7.0 files were inspected and have MIT/permissive notices:

- [intel_lrc.c](https://github.com/torvalds/linux/blob/v7.0/drivers/gpu/drm/i915/gt/intel_lrc.c): logical context layout and generation-specific setup.
- [i915_gem_execbuffer.c](https://github.com/torvalds/linux/blob/v7.0/drivers/gpu/drm/i915/gem/i915_gem_execbuffer.c): validation, residency and submission semantics.
- [i915_pci.c](https://github.com/torvalds/linux/blob/v7.0/drivers/gpu/drm/i915/i915_pci.c): ADL-N IDs use `adl_p_info`; Gen12 features.

This makes those specific files candidates for carefully scoped reuse. Their
Linux memory, scheduler, reservation, DRM and power dependencies still need
individual design and licensing review. The finding does not license all i915,
Xe or Linux dependencies under MIT. No code is imported by this document.

The existing RenkuOS Gen9 draft render-node work must be reviewed separately:
Gen9 context layouts/workarounds and its Genode-derived licensing cannot be
assumed suitable for Alder Lake-N. Prefer Intel's own documented interfaces and
permissively licensed implementation details for any new Gen12 work.
