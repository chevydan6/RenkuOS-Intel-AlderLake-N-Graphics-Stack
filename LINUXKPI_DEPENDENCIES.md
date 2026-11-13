<!-- SPDX-License-Identifier: MIT -->
# Build-derived dependency inventory

| Interface | Status | Actual implementation / evidence |
|---|---|---|
| Complete drm_mm structs and algorithms | implemented, CPU tested | Unchanged v6.6 drm_mm.c/.h, complete upstream allocator, 019/020 |
| rbtree, augmented and interval trees | implemented, CPU tested | Original Linux implementation; no replacement allocation algorithm |
| list/hlist types and operations | implemented | Original list.h with native-width types; no implicit synchronization |
| u8/u32/u64/phys_addr_t/resource_size_t | implemented | Fixed-width types, real native phys_addr_t; x86-64 required |
| GEM_BUG_ON | implemented adapter | Calls real RenkuOS panic; kernel resolution remains required |
| WARN / DRM diagnostics | implemented adapter | Native dvprintf; real DRM printer callback |
| BIT, power-of-two, min/max, div64 remainder | implemented | Scalar helpers, alignments tested in actual allocator |
| READ/WRITE_ONCE, acquire/release | implemented for reached scalar types | Compiler atomics, also used by RenkuOS SupportDefs; not a general Linux memory-model port |
| test_bit, non-atomic set/clear, clear_bit_unlock | implemented | Atomic relaxed read/release clear; caller serializes drm_mm mutations |
| RCU pointer publication | implemented publication only | Release store; RCU grace periods/reclamation unresolved; allocator uses non-RCU tree routines |
| __iomem | implemented annotation only | Does not map MMIO or make ordinary loads ordered I/O |
| ERR_PTR | implemented in ring slice | Internal Linux error pointer; no native status conversion yet |
| Linux ENOSPC | implemented internal namespace | 28, separate from negative RenkuOS status_t; no exported ioctl boundary |
| KernelExport.h/SupportDefs.h | implemented build integration | Actual pinned target source headers, host libc excluded (020) |
| spinlock/mm_types/slab/seq_file/stacktrace includes | excluded APIs | Include wrappers expose no fake APIs; selected release allocator references none |
| CONFIG_DRM_DEBUG_MM/CONFIG_DEBUG_LIST | excluded configuration | Unsupported debug backends rejected; ASan/UBSan host allocator checks enabled |
| Module export tables | excluded | Plain relocatable objects; EXPORT_SYMBOL emits no loader table, no module claim |
| intel_ring/i915_request layouts | adapted subset only | Explicit private layouts, not complete Linux structs or ABI |
| i915_request_timeline / wait_for_space | unresolved | Undefined symbols; never return fake completion |
| ring create/pin/unpin/free | excluded from compiled slice | Original source retained, full-file compilation still fails |
| Linux atomic/refcount/kref | unresolved | Whole ring reaches asm/atomic.h via drm_gem.h (018) |
| PCI/MMIO/DMA/GT initialization/contexts/execlists | unresolved integration | Not provided by CPU allocator; no speculative adapter implementations |

Caller must serialize drm_mm operations. No general spinlock, page allocation,
DMA mapping, scheduler, fence or RCU reclamation implementation has been claimed.

FreeBSD references consulted: drm-kmod README blob
 a39b6dfa5bb526205b8fb1a30db11196e752c4ba (matching kernel sources required),
LinuxKPI linux/types.h blob faec55a72e57a5fa1e9f5b73407d11ec8d72808a
(native types/VM address mappings). No FreeBSD code was imported, and these
references are not claimed to be a matched driver/kernel release baseline.
