<!-- SPDX-License-Identifier: MIT -->
# Build-derived dependency inventory
| Interface | Disposition | Evidence / backend |
|---|---|---|
| u32/u64, bool, size_t | implemented for slice | Standard fixed-width C types; host object compiled |
| GEM_BUG_ON | implemented adapter, native link unresolved | Calls actual RenkuOS panic; no disabled assertion |
| unlikely, IS_ALIGNED, is_power_of_2 | implemented | Local compiler/pure helpers |
| memset64/memset32 | implemented | Actual element writes, no fake operation |
| ERR_PTR | implemented for slice | Signed error to pointer; native behavior untested |
| intel_ring / i915_request layout | adapted subset | Explicit private layouts, no Linux ABI claim |
| i915_request_timeline | unresolved | Undefined symbol in object |
| wait_for_space / request retirement | unresolved | renku_ring_wait_for_space undefined, no success stub |
| KernelExport.h / SupportDefs.h | unresolved build environment | Native attempt 005 fails missing SupportDefs.h |
| GEM internal/lmem/object includes | unresolved | Whole-file attempt 001 fails first include |
| Ring create/pin/unpin/free | excluded from compiled slice | Original source retained; allocation, GGTT and object lifetime not ported |
| Debug GEM poison config | excluded | CONFIG_DRM_I915_DEBUG_GEM=0 for slice |
| PCI, DMA, MMIO, fences, GT init, contexts, execlists | unresolved integration | Not reached by selected slice; no speculative interfaces implemented |

FreeBSD references consulted via GitHub connector:
drm-kmod README blob a39b6dfa5bb526205b8fb1a30db11196e752c4ba
requires matching kernel sources; LinuxKPI linux/types.h blob
faec55a72e57a5fa1e9f5b73407d11ec8d72808a uses native sys types and VM address
types. No FreeBSD implementation was imported and these references are not
claimed to form a matched driver/kernel baseline.
