<!-- SPDX-License-Identifier: MIT -->
# Demonstrated blockers at the continuation checkpoint

- Whole original intel_ring.c now parses ww_mutex and DMA-fence declarations and
  fails in DRM VMA manager interfaces: rwlock_t, read_lock/read_unlock/rwlock_init,
  then __user/raw_spinlock completion and IDR/radix-tree header compatibility (062).
  Earlier memset/__u64/VM-area declarations from 046 are now supplied. The native page constants
  are checked, but Linux VM mapping/unmapping and DMA allocation are not implemented.
- Original ww inline code emits an ELF object, but ww_mutex_lock and
  ww_mutex_lock_interruptible remain undefined in the actual partial link (047).
  Deadlock avoidance, waiter wakeup/cancellation, RCU lifetime and real fence
  waiting/signaling remain unresolved, not silently simulated.
- Request timeline and ring space waiting/retirement remain undefined. New native
  task/time adapters import real find_thread and system_time_nsecs. Registered
  task scopes require caller lifetime discipline; they are not general scheduler
  objects or task references. Scoped task/time/lock paths now pass actual guest
  kernel worker tests; GPU retirement remains unimplemented.
- Native-compiled execution object still imports i915_request_timeline and
  renku_ring_wait_for_space (native-vm-006/execution-imports.txt). No execution-ready
  backing, real context submission or completion connects that object. The new
  memory-only native GGTT sandbox validates coordinated owned ranges and CPU
  aperture readback/release, but deliberately prohibits device publication.

Resolved, evidence retained:
- Missing VM/toolchain: actual RenkuOS build00081 runs under KVM with matching
  SDK. Native driver passes 16 × 62 checks; UNINIT status=0 confirms teardown.
- Physical target access: restored using the user-supplied NanoKVM connection.
  Actual N100 hrev60072+518 compilation and kernel-service suite pass another
  16 × 62 checks with matching GCC13.3/SDK. No GPU-execution result is implied.
- Kernel-worker join panic (066): exact guest source establishes no cached
  death entry for kernel threads. Completed, absent-ID late joins are handled
  explicitly; live/unknown-error cases remain fatal. Final native tests pass.
- Reinstall devfs ENOENT (067): asynchronous publication now has a bounded wait.
  Native allocation/semaphore cleanup and invalid-resource tests pass.
- Shell Git/DNS failures from first turn: clone now works.
- Missing SupportDefs.h: real headers populated; StorageDefs.h search path fixed.
- ENOSPC/min/max macro conflicts with target headers (013): scoped Linux
  namespace now deliberately separated from native status_t (not ABI-converted).
- Host ASan/UBSan link failure (014): runtimes locally extracted; test passes
  with both sanitizers (019), no checks disabled.
- Previous asm/atomic.h/refcount compilation boundary is resolved by the scoped
  atomic adapter and original refcount code; see 029. Signed-overflow sanitizer
  failure (027) is resolved by Linux's required -fno-strict-overflow build flag,
  preserving all saturation cases and enabled sanitizers (028).
- Previous atomic_long, mutex named initialization/state and current-identity
  compile failures from 033 are resolved for scoped execution, tested in 047/048.
  errno/error-pointer namespace failures are resolved; all original pointer codes
  and preservation of native B_* status aliases passed the target-header probe.
- The original batch-start emitter and ring slice link and execute CPU packet
  construction (056/057), but still use private request/ring layouts. They lack
  real GT/context/mapping/submission/completion integration and cannot be passed
  full Linux i915 request objects or counted as GPU command execution.

- The mapping sandbox exposes no per-device DMA address or submission lifetime.
  Its successful CPU aperture access does not establish IOMMU identity, engine/GuC
  TLB behavior, PAT/MOCS/cache visibility or retirement/reset ownership. Existing
  native GT/context work still needs integration; first GPU execution is absent.

Mapping-specific resolved failures (physical-mapping-001): direct aperture access
faulted under SMAP; a native kernel-only clone fixes that while preserving WC/UC
and original GART free addresses. Final physical mapping runs pass 8 × 38,927 checks.
The diagnostic's asynchronous devfs removal now has a bounded wait for current
unload evidence and an absent device. No functional mapping assertion was removed.

## Linux DMA port boundary

Linux coherent allocation/free and chained SG map/unmap now have an actual native
bus_dma backend with physical kernel tests. This resolves the absence of those
scoped API adapters, not i915 PCI attachment or GPU DMA activity. Atomic coherent
allocation, general Linux page/folio and SG allocators, IRQ-safe streaming maps,
IOMMU setup and full device teardown integration remain unsupported.

Complete original i915 GEM builds still fail: whole-gtt-001.log reaches missing
gfp_t propagation, log2/bit helpers and compiler-header collisions; whole-phys-001
reaches annotation collision and asm/linkage.h. Complete-ring builds still need
rwlock/completion and the broader DRM/GEM dependency closure. Keep those source
files unchanged as compatibility work proceeds. No complete i915 driver loads.

## VMA continuation

The original complete DRM VMA offset manager now builds and runs with native
reader/writer locks and heap services. Native four-run tests and actual unload
pass. Earlier gfp_t propagation, annotation/stdarg/log2 conflicts and plain
read/write lock support are resolved for the tested scope.

Later whole GEM/ring failures are retained in physical-vma-001: missing s8/u16/
s16/s32 aliases, asm/div64.h, raw spinlock/wait interfaces, lock_class_key collision
and missing external sparse-checkout UAPI headers. VMA offset management does not
implement user mmap, page faults, GEM lifecycle, fences or request retirement.

## Completion continuation

Complete original Linux completion/swait algorithms now have a tested native
event/deadline backend for caller-owned registered entry scopes. Timeout, counted
wake/broadcast and real interruptible/ordinary-signal killable behavior pass on
the N100. This is not a general Linux scheduler/signal subsystem, task-reference
model, GPU fence integration or proof of cancellation teardown. First wait-event
allocation still fails fatally on resource exhaustion; production entry/error
handling and scope ownership must be wired before a full driver is exposed.

Whole GEM/ring files still fail with the actual RenkuOS compiler. Current errors
include macro/attribute namespace collisions, early init/might_sleep declarations
and missing architecture/preemption interfaces. See physical-completion-001/
whole-build/. The earlier scalar types/division/raw-spin/class-key obstacles are
resolved for the tested compilation environment. GPU submission remains absent.


## Preemption/helper continuation (current boundary)

Original canonical Linux helper imports resolve the previous ALIGN/roundup/min/
compiler-attribute collisions. Early might_sleep/init visibility is supplied.
Native spin/rw/preemption scopes share a real per-CPU IRQ backend, validated on
RenkuOS, with shutdown rejecting active users. Physical IRQ masking is stronger
than task preemption; callers must not block and must keep sections short.

The current physical whole-file failures are preserved in
experimental/i915/evidence/physical-preempt-001/whole-build: GTT lacks asm/ioctl.h;
physical GEM collides on MAJOR/MINOR/MKDEV, lacks RCU helpers and asm/cache.h;
ring lacks bitmap/bit-scan primitives, loff_t and cpumask layout support. Follow
these compiler-derived dependencies using original Linux code and actual OS
services. No Linux interrupt-context counters, complete RCU lifetime system,
full i915 attachment, GPU submission/completion or Mesa integration is supplied.

## Bitmap/search continuation (current boundary)

Original Linux find_bit/hweight and generic bitops now compile and pass native
oracle and old synchronization regressions. Bitmap layout and Linux loff_t are
supplied. External bitmap allocation/parsing operations are still unresolved.
Use public nth searches: original private size0 nth entry is unsafe; its warning
is retained rather than changing upstream source. See completion/README.md.

Native whole ring now fails at cpumask configuration (UP num_online_cpus conflicts
with the actual native service) and asm/cache.h. Do not fake a one-CPU system or
substitute an invented CPU-mask layout. GTT reaches early init/random/static-key/
atomic helpers, U32_MAX and asm/param.h. Physical GEM retains MAJOR/MINOR/MKDEV
namespace, RCU and cache-header dependencies. Actual logs are in physical-bitmap-001.

## CPU-mask continuation (current boundary)

Original cpumask helpers now use SMP configuration and a real native CPU/enabled
snapshot (64-CPU capacity, rejects oversized state). Native masks/operations,
constant lookup, cache alignment and reached atomic/type/limit helpers pass.
This requires serialized, fixed-topology ownership. CPU enable/disable events,
affinity, cross-CPU execution and NUMA integration remain unavailable.

Current actual native compiler failures: ring requires generated/bounds.h for
Linux VM page layout; GTT requires asm/spinlock_types.h for raw lock types;
physical GEM requires device-number namespace, RCU lifetime/lookup, bit unlock
and bounds integration. These need real layout/service adaptation, not invented
page/CPU objects or no-op success. See physical-cpumask-001/whole-build. Existing
find_bit private zero-size caveats and compiler warnings remain explicit.

## Raw-lock continuation (current boundary)

Original raw/regular/RW structures now use actual native architecture storage,
with typed APIs and no raw/regular alias. Complete original bit-spin algorithms
and the reached release-store bit unlock pass native tests. Original __bitmap_equal
is supplied as a verified complete-function extraction; other external bitmap
services remain unresolved. The fixed-topology and preemption limits still apply.

Current native whole-file failures remain VM generated bounds/page layout, GTT
logging/task-name/va_format declarations, and physical-GEM device-number namespace/
RCU lifetime dependencies. See physical-raw-lock-001/whole-build. Do not create
fake bounds, substitute page layouts, or make RCU a no-op to advance compilation.


## 2026-11-14: scoped RCU, logging and device IDs

Native reader/grace-period exclusion, bounded printk, real task-name snapshots
and original unsigned Linux device-ID helpers now pass physical N100 testing.
All107 upstream imports remain unchanged. Four completion runs of46934 checks,
plus four VMA/KPI regression runs, total190576 checks with zero failures and
actual diagnostic unload. Eight RCU workers exercise nested readers, waiting
grace periods and four real frees after exclusion. Host sanitizer/model and
mutation controls pass; evidence records the exact sources and scope.

RCU is restricted to short nonblocking task readers with IRQ-enabled,
preemptible outer entry and fixed topology. It uses stronger native RW locking;
callback queues and general Linux RCU progress semantics remain unsupported.
All three complete original ring/GEM compiler attempts now reach Linux
VM generated/bounds.h. The earlier RCU-read, logging/task-name and device-number
errors are resolved at this boundary. Real VM/page-layout integration is next;
full i915 compilation and GPU execution remain unproven. See
experimental/i915/evidence/physical-rcu-001 and completion/README.md under the
experiment for exact contracts, unsupported services and validation evidence.


## 2026-11-14: lockref/GFP checkpoint

Original lockref fallback and GFP flags/blocking predicate are ported; all111
imports remain unchanged. Native completion passes4x47056 checks and actual
unload, including eight lockref workers/32000 iterations. The old native VMA
rendezvous FAILED on run2; the revised two-CPU callback fixture has target-header
compile validation only, awaiting USB input recovery and native retest. Do not
report this checkpoint as passing all regressions.

Cross-source ring/GTT builds now expose real Linux process-mm/task dependencies
in sched/mm.h; physical GEM exposes seqlock types and asm/stat.h. No new native
whole-file compiler result is claimed. The lockref-only bounds shim supplies no
VM bounds or guessed layouts and rejects every VM inclusion outside its scope.
See experimental/i915/evidence/physical-lockref-001 and lockref-host-001.


## 2026-11-14: lockref/VMA recovery validation

The pending VMA fixture now passes actual RenkuOS compilation and four650-check
runs, with two readers, zero worker errors,48000 stress operations and unload0.
The same native lockref/completion source rebuilt and passes four47056 checks;
all eight runs total190824 checks, zero failures. Native source checks pass
before/after. Whole native ring/GTT now reach sched/mm.h process-mm/task services;
physical GEM reaches seqcount/seqlock and asm/stat.h. Full i915 build/GPU execution
remain absent. See experimental/i915/evidence/physical-lockref-recovered-001.
