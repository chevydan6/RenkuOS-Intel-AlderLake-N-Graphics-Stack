<!-- SPDX-License-Identifier: MIT -->
# Build-derived dependency inventory

| Interface | Status | Actual implementation / evidence |
|---|---|---|
| Complete drm_mm structs and algorithms | implemented, native VM tested | Unchanged v6.6 drm_mm.c/.h; host stress 019/020 and native-vm-006 allocation/overlap/cleanup |
| rbtree, augmented and interval trees | implemented, native VM tested | Original Linux implementation through drm_mm; no replacement allocation algorithm |
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
| mm_types/slab/seq_file/stacktrace includes | excluded APIs | Include wrappers expose no fake APIs; selected release allocator references none |
| CONFIG_DRM_DEBUG_MM/CONFIG_DEBUG_LIST | excluded configuration | Unsupported debug backends rejected; ASan/UBSan host allocator checks enabled |
| Linux module export tables | excluded | EXPORT_SYMBOL emits no Linux loader table; native test driver uses actual RenkuOS entry points |
| intel_ring/i915_request layouts | adapted subset only | Explicit private layouts, not complete Linux structs or ABI |
| i915_request_timeline / wait_for_space | unresolved | Undefined symbols; never return fake completion |
| ring create/pin/unpin/free | excluded from compiled slice | Original source retained, full-file compilation still fails |
| atomic_t/refcount/kref | implemented reached subset | Original Linux headers/refcount.c, native atomic primitives; 028/029/031 |
| WARN_ONCE | implemented | Per-callsite atomic warning suppression, actual native diagnostic |
| Linux mutex init/lock/unlock/destroy | implemented, native VM tested | Native semaphores; two actual kernel workers, contention/cleanup in native-vm-006 |
| spin_lock/init/unlock and IRQ-save/restore | implemented, native VM tested | Native spinlocks with IRQ exclusion; plain spin_lock is stronger than Linux |
| PREEMPT_RT/rt_mutex | excluded configuration | No RT methods provided; non-RT ww_mutex uses struct mutex; RT build fails |
| atomic_long stamp increment | implemented | Native atomic_add64; target-header wrap check passes |
| ww acquire context/header | compiled original inline code, CPU context tested | ww_mutex.o is ELF; 40000 original contexts on 8 host threads |
| ww_mutex_lock/interruptible/unlock/trylock | unresolved | No ordinary-lock success substitution; lock symbols undefined |
| Named mutex init/state | implemented, native VM tested | Named native semaphore; atomic held-state snapshot |
| current / task identity | implemented scoped identity | Real find_thread ID with caller-owned registration record; not a scheduler/task lifetime service |
| Linux errno/error pointers | implemented | Byte-exact headers; 4095 pointer codes and 6 native status aliases tested |
| ktime_get/ktime_t | implemented, native linked | system_time_nsecs import; direct ktime_get runtime not exercised by native test; completion deadlines use native system_time |
| RCU callback metadata/acquire pointer loads | represented/implemented only | Reader protection, grace periods and reclamation unresolved |
| smp_mb/cpu_relax | implemented | Real CPU/compiler seq-cst barrier and x86 pause |
| printk/seqlock | excluded APIs | Reached headers call no methods; wrappers expose no fake logging/sequence operations |
| Native PAGE_SIZE/PAGE_SHIFT | implemented constants | Compile-time B_PAGE_SIZE check; Linux struct page APIs unresolved |
| Scoped native wired-page backing | implemented, native VM tested | create_area/get_memory_map_etc/delete_area; 64 allocations/frees; physical addresses are not DMA mappings |
| Scoped native completion/deadlines | implemented, native VM tested | Semaphore token lifecycle, monotonic deadline/retry; Linux completion/waitqueue semantics unresolved |
| Native driver/module glue | implemented, native VM tested | device_hooks, _KERNEL_ import library/version glue; INIT/UNINIT observed; not a blanket Linux module API |
| Kernel worker lifecycle | implemented for native test | spawn/resume/wait; completed absent-ID late joins handle kernel threads without cached death entries |
| unmap_mapping_range | unresolved | Declaration only; no successful unmap stub |
| DRM VMA rwlock and VM interfaces | unresolved | Current compiler errors in 062; basic memset/__u64/VM-area forward declarations supplied |
| PCI/MMIO/DMA/GT initialization/contexts/execlists | unresolved integration | Not provided by CPU allocator; no speculative adapter implementations |

Caller must serialize drm_mm operations. No full Linux locking family, page allocation,
DMA mapping, scheduler, fence or RCU reclamation implementation has been claimed.

FreeBSD references consulted: drm-kmod README blob
 a39b6dfa5bb526205b8fb1a30db11196e752c4ba (matching kernel sources required),
LinuxKPI linux/types.h blob faec55a72e57a5fa1e9f5b73407d11ec8d72808a
(native types/VM address mappings). No FreeBSD code was imported, and these
references are not claimed to be a matched driver/kernel release baseline.

Refcount target compilation requires Linux's -fno-strict-overflow semantics.
Upstream signedness diagnostics are excluded with -Wno-sign-compare, not source
rewrites. kref/refcount original headers retain all saturation checks. Locked
refcount host tests execute pthread backends, not native kernel primitives;
userspace IRQ paths abort. Public/native syscall status conversion remains absent.

Native errno aliases B_BUFFER_OVERFLOW/B_TOO_MANY_ARGS/B_FILE_TOO_LARGE/
B_RESULT_NOT_REPRESENTABLE/B_DEVICE_NOT_FOUND/B_NOT_SUPPORTED are frozen before
Linux errno definitions replace E* names. This preserves native statuses within
a mixed translation unit; external ioctl/syscall conversion is still unresolved.
Task registration must span every referencing acquire context. There is no
implicit task allocation/cache, fake current pointer, schedule or wakeup API.

| Execution-slice interface | Status | Evidence |
|---|---|---|
| gen8_emit_bb_start/noarb | compiled original bodies, CPU tested | Real ring_begin integration; 056/057 |
| intel_ring_advance | original inline body retained | Exact bounds/alignment assertions |
| intel_gpu_commands definitions | imported original | Byte-exact source; no rewritten opcodes |
| nopreempt query | adapted software property | Private request attribute, not a general scheduling implementation |
| trusted I915_DISPATCH_SECURE flag | adapted constant | Linux v6.6 intel_engine_types.h BIT(0), no public userspace dispatch |
| sign_extend64/GENMASK/upper-lower bits | implemented arithmetic | Original canonical-address helpers and >4-GiB packets tested |

The slice's test-only timeline/wait definitions abort if reached. They are not
linked into the native component set and do not emulate completion. Native ring
waiting/retirement is still unresolved. The full i915 request layout is distinct.
