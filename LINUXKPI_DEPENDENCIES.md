<!-- SPDX-License-Identifier: MIT -->
# Build-derived dependency inventory

| Interface | Status | Actual implementation / evidence |
|---|---|---|
| Complete drm_mm structs and algorithms | implemented, VM + N100 kernel tested | Unchanged v6.6 drm_mm.c/.h; host stress 019/020 and native-vm-006 allocation/overlap/cleanup |
| rbtree, augmented and interval trees | implemented, VM + N100 kernel tested | Original Linux implementation through drm_mm; no replacement allocation algorithm |
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
| Linux mutex init/lock/unlock/destroy | implemented, VM + N100 kernel tested | Native semaphores; two actual kernel workers, contention/cleanup in native-vm-006 |
| spin_lock/init/unlock and IRQ-save/restore | implemented, VM + N100 kernel tested | Native spinlocks with IRQ exclusion; plain spin_lock is stronger than Linux |
| PREEMPT_RT/rt_mutex | excluded configuration | No RT methods provided; non-RT ww_mutex uses struct mutex; RT build fails |
| atomic_long stamp increment | implemented | Native atomic_add64; target-header wrap check passes |
| ww acquire context/header | compiled original inline code, CPU context tested | ww_mutex.o is ELF; 40000 original contexts on 8 host threads |
| ww_mutex_lock/interruptible/unlock/trylock | unresolved | No ordinary-lock success substitution; lock symbols undefined |
| Named mutex init/state | implemented, VM + N100 kernel tested | Named native semaphore; atomic held-state snapshot |
| current / task identity | implemented scoped identity | Real find_thread ID with caller-owned registration record; not a scheduler/task lifetime service |
| Linux errno/error pointers | implemented | Byte-exact headers; 4095 pointer codes and 6 native status aliases tested |
| ktime_get/ktime_t | implemented, native linked | system_time_nsecs import; direct ktime_get runtime not exercised by native test; completion deadlines use native system_time |
| RCU callback metadata/acquire pointer loads | represented/implemented only | Reader protection, grace periods and reclamation unresolved |
| smp_mb/cpu_relax | implemented | Real CPU/compiler seq-cst barrier and x86 pause |
| printk/seqlock | excluded APIs | Reached headers call no methods; wrappers expose no fake logging/sequence operations |
| Native PAGE_SIZE/PAGE_SHIFT | implemented constants | Compile-time B_PAGE_SIZE check; Linux struct page APIs unresolved |
| Scoped native wired-page backing | implemented, VM + N100 kernel tested | create_area/get_memory_map_etc/delete_area; 64 allocations/frees; physical addresses are not DMA mappings |
| Scoped native completion/deadlines | implemented, VM + N100 kernel tested | Semaphore token lifecycle, monotonic deadline/retry; Linux completion/waitqueue semantics unresolved |
| Native driver/module glue | implemented, VM + N100 kernel tested | device_hooks, _KERNEL_ import library/version glue; INIT/UNINIT observed; not a blanket Linux module API |
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

Physical checkpoint: the same native service suite passes on the actual N100
hrev60072+518 with matching SDK/GCC13.3; see evidence/physical-n100-001 under
experimental/i915. This validates the scoped page/lock/task/deadline/module
adapters, not DMA mapping, Linux waitqueues or GPU execution.

## Memory-only native GGTT bridge

| Interface | Status | Evidence |
|---|---|---|
| Coordinated owned GGTT backing | implemented isolated candidate, N100 tested | Existing native owner/GART reused; own Request lease, four checked non-reserved ranges; physical-mapping-001 |
| Kernel CPU aperture alias | implemented, N100 tested | clone_area of same device cache, kernel-only protection; WC/UC preserved; original GART free addresses retained |
| N100 GGTT invalidation callback | implemented native candidate | UC0x101008 bit0 write/posting read, existing native wbinvd retained; actual CPU aperture tests, not engine/GuC TLB validation |
| Mapping cleanup ownership | implemented memory-only scope | Per-object successful-free records, alias deletion, retained failures; UBSan fault tests and8×38927 actual native checks |
| Per-device DMA/cache/execution lifetime | unresolved | No CPU-physical=DMA substitution, publication API, valid context, submission or retirement |

This extends the native foundation; it imports no additional Linux source body
and leaves the existing 25 Linux imports unchanged. It does not expose general
Linux GEM/VMA or DMA services. A caller must finish every CPU access before end;
GPU use is prohibited through this memory-only interface.

## Original i915 DMA API prerequisites

The DMA adapter supports the Linux API used by the byte-exact imported
i915_gem_gtt.c and i915_gem_phys.c. Unchanged Linux SG layout/sg_next/sg_init_table
are used; RenkuOS bus_dma supplies native callback addresses. Coherent allocations
have CPU/DMA containing-order alignment and strict lifetime checks. Streaming
mapping requires wired contiguous per-entry spans and rolls back partial failure.

Physical evidence: four ×45,116 CPU/API checks, no failures, actual unload. The
opaque device test key has no PCI attachment; this is not GPU DMA proof. General
Linux page/folio allocation, remaining SG functions, PCI integration and full
GEM/ring compilation remain dependencies. See dma/README.md and the preserved
whole-source compiler errors in evidence/physical-linux-dma-001/.

## DRM VMA offset manager

Complete original drm_vma_manager.c/.h now compile and run over original drm_mm/
rbtree. Real native rw_spinlock adapters support plain read/write locking with
per-CPU nested IRQ state; startup allocates a wired CPU-count-sized table and
shutdown rejects live scopes. Sleeping kmalloc/native deferred kfree support
VMA access records; atomic allocation and general slab caches remain unsupported.
Original compiler_types/log2 imports plus compiler stdarg/ABI wrappers resolve
earlier header collisions. See vma/README.md and physical-vma-001 evidence.

## Original completion and simple wait queues

Whole completion.c/swait.c and original headers now build/run through native IRQ
locks, semaphore events, monotonic deadlines and registered entry scopes. The
actual N100 tests cover counted wakes, multiple waiters, broadcasts/reuse, timeout
and user-thread signal handling; no GPU completion is connected. Scopes own
lazily created native events and destroy them before unregister/unload. HZ=1000
is explicit; general task/signal scheduling and Linux blkio accounting are not
implemented. See completion/README.md for exact limitations and reproduction.

Original generic division/kconfig/lockdep-off/UAPI helpers and native atomic-bit
operations reduce the next whole-source dependency set. Fully materialized
reference headers are verified against the pin before native compiler attempts.


## Shared native IRQ/preemption scope backend

One wired CPU-sized table now serves native spinlocks, rwlocks and preempt_disable.
Physical IRQ masking prevents migration for each short section; requested restore
state remains deferred until the final live scope exits. Nested _irq/irqsave and
non-LIFO lock/preemption overlap pass on the N100, as do active-scope teardown
rejection and original completion/VMA regressions. Startup/shutdown serialize
users and CPU topology remains fixed. Interrupt-context classification stays
unresolved rather than inventing Linux IRQ state. See completion/README.md.

The import manifest verifies 65 original files, including canonical compiler
attributes/build_bug/minmax/alignment/arithmetic/endian/swab helpers. Native C
library declarations precede Linux macro selection. Complete GEM/ring attempts
now reach bitmap/cpumask, RCU, device-number and architecture ioctl/cache headers;
these remain dependencies, with actual target errors in physical-preempt-001.

## Original bitmap search and bit operations

87 unchanged Linux imports now include complete find_bit.c/hweight.c and public
bitmap/find/bitops headers. Original generic bit scans and non-atomic mutations
replace local helpers, with native/compiler atomic operations underneath shared
word mutations. Original generic ioctl numbering does not implement dispatch.
Native four-run search/helper/completion/preemption tests pass185,556 checks;
VMA/KPI regressions and host sanitizer/oracle/mutation controls pass separately.
See physical-bitmap-001 and completion/README.md for compiler warnings, private
nth-entry preconditions and remaining external lib/bitmap.c functions.

Next whole-file dependencies are CPU-mask configuration/cache headers, random/
static-key atomic helpers, U32_MAX and asm/param.h, plus physical-GEM RCU/device
number namespace integration. No GPU algorithm was replaced to reach this point.
