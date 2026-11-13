<!-- SPDX-License-Identifier: MIT -->
# Comparison at first compilation checkpoint
Native GPU branch already supplies tested HDMI, 64-bit GGTT changes, physical
memory/forcewake/reset evidence, and CPU context/descriptor helpers. Preserve them.

Linux reuse demonstrably supplies compilable original ring arithmetic and packet
reservation bodies. Whole-file reuse immediately encounters GEM dependencies.
The successful object uses an extracted subset and private layouts: it supports
the feasibility of selective algorithm reuse, not the superiority of full LinuxKPI.

No measured reduction in work to first GPU execution or Mesa has been established.
GT initialization, mapped owned memory, context construction, real submission and
completion remain unintegrated. Native build validation is also outstanding.
Next comparison should measure dependencies and adapter code needed to compile
the complete ring file and wire real request retirement, alongside equivalent
existing native mechanisms. File count alone is not a decision criterion.

## Complete DRM allocator result
The full Linux drm_mm algorithm and its actual tree implementation compile
against RenkuOS headers and pass allocator stress with ASan/UBSan. This is stronger
reuse evidence than extracted ring arithmetic: a complete existing dependency
works through a small compatibility surface without rewritten allocation logic.
The allocator tracks virtual ranges only. Native physical/GART ownership and
Gen12 PTE programming are still required; no performance/effort win on GPU
submission, completion or Mesa has been measured.

Whole ring reuse now demonstrably reaches GEM refcount/atomic dependencies
before request and VM integration. This supports continuing a bounded experiment,
not selecting full LinuxKPI over the existing physically tested native foundation.

## Refcount dependency result
The whole Linux refcount implementation now compiles with a small native atomic
and locking surface, retaining saturation and release/acquire algorithms. CPU
concurrency/saturation tests pass. Native kernel execution is not validated.
This advances complete-source reuse beyond the allocator but introduces concrete
OS semantics (mutex lifecycle and IRQ exclusion) absent from the initial slice.

Whole-ring reuse still brings dma_resv, wound/wait deadlock avoidance, stable task
identity and dma-fence machinery into the closure. These are now compiler-derived
dependencies rather than a speculative inventory. They remain more work than
simple typedefs or no-op wrappers. The evidence still does not select full
LinuxKPI over native execution or establish a reduced path to GPU/Mesa execution.

## Wound/wait prerequisite result
Original ww context code now compiles to a target-header ELF object and passes
CPU context tests after adding native stamp, semaphore state and scoped task
identity services. Error-pointer helpers also execute correctly on the host with
native-header error-domain preservation. This is a concrete dependency reduction.

The linker still requires real ww lock algorithms; header/context compatibility
has not solved multi-object deadlock avoidance. Fence waiting/RCU and DRM VMA
memory interfaces remain actual boundaries. Native task-scope lifetime and IRQ
semantics also need kernel runtime evidence. No reduction in measured time to
GPU execution or Mesa has been established; no path superiority is claimed.

## Direct execution-component reuse result
Two complete original Linux batch-start emitter bodies now compile against
RenkuOS headers and feed the existing original ring reservation code. Literal
packet/guard/wrap tests pass both under sanitizers and using the target-built
objects. This is command-construction reuse, not just generic compatibility code.

The selected extraction needs only a software request attribute and the ring
interface, while whole-file compilation still pulls in GEM, VMA, IDR, completion,
ww locking and RCU/VM services. This is evidence that the adaptation boundary
substantially changes the dependency burden. It does not measure first-command
hardware effort: native GT/context/mapping/submission/completion integration is
still missing, so neither full LinuxKPI nor the extracted path is declared better.

## Actual native kernel integration result

The experiment now builds and loads a real driver in RenkuOS build00081 with
the matching SDK and native GCC15.3. Original Linux allocator/tree/refcount and
ww context algorithms run against native kernel pages, mutexes, spinlocks,
threads, task identity and time. Sixteen invocations × 62 checks pass, including
concurrency, timeout/error handling and cleanup; actual unload is recorded.
The earlier absence of native runtime evidence is resolved for this subset.

Runtime exposed concrete adaptation costs: kernel threads do not retain death
entries, and device publication is asynchronous. Both failed experiments are
preserved and corrected. These are manageable OS integration details, but show
why compiling headers alone did not establish correctness.

The original execution slices also compile with the actual guest SDK into a
partial execution object. Their unresolved timeline/wait symbols, missing GPU
mappings and absent real context/submission/completion still prevent a physical
store. Native wired physical pages do not solve DMA or GPU address translation.
No CPU-service timing here measures GPU throughput, compatibility overhead,
Mesa readiness or time to first execution. Full LinuxKPI superiority remains
unproven against the native branch's existing physical GT/memory/context work.

## Physical N100 service result

The same source now builds and runs on the actual four-core N100, using its
older hrev60072+518 kernel, matching SDK and GCC13.3. Sixteen runs × 62 checks
pass again, with actual module teardown and persistent HDMI console output.
This removes uncertainty about these selected CPU adapters being VM-only or
dependent on the newer guest SDK. GPU-address ownership, GT/context setup,
submission, retirement and the broader Linux dependencies remain unresolved.
Physical CPU-service success still does not establish GPU performance overhead
or a shorter path to Mesa, and does not select the full Linux port as superior.

## Owned native GGTT integration result

The isolated experiment now reuses the native retained owner and GART to supply
four owned non-reserved GGTT buffers and kernel CPU aperture aliases. The N100
passes 8 × 38,927 checks for CPU-pattern readback, range separation and release, with
actual diagnostic unload and persistent HDMI. Runtime exposed an SMAP permission
boundary; native device-cache cloning fixes it without treating the original
user-permission aperture as ordinary kernel RAM. Failure/retry tests preserve
resident ownership, including separate original allocation and CPU alias domains.

This progress comes from the existing native memory foundation, plus a scoped
interface and a native N100 GGTT invalidation correction. No additional Linux
algorithm is imported. It reduces the experiment's missing backing/CPU-access
surface, but establishes neither per-device DMA admission nor execution-ready
engine translations, caches, context, submission or retirement. No GPU workload
or performance has been measured and full LinuxKPI superiority remains unproven.

## Linux DMA compatibility result

Original i915 GEM source now has tested coherent/streaming DMA API prerequisites
backed by existing RenkuOS bus_dma. New work is OS adaptation; GPU algorithms stay
upstream. Four native N100 runs pass 180,464 checks and unload cleanly. Whole GEM
source still fails on the remaining Linux/DRM closure. This advances portability
but supplies no GPU workload, throughput measurement or Mesa readiness claim.

## Whole DRM VMA dependency result

The unchanged complete DRM VMA offset manager now compiles and executes on the
N100 using existing native rw_spinlock/heap services. Four runs pass 2,584 checks
and 48,000 lock operations, including overlapping readers and IRQ restoration.
This reduces the actual i915 dependency work without rewriting GPU algorithms.
Complete GEM/ring builds still fail on later compatibility interfaces; no GPU
execution or Mesa support follows from this CPU-only DRM module result.

## Original completion reuse result

The complete Linux completion and simple-wait-queue algorithms now execute on
the N100 over existing native semaphore/IRQ/time services. Four runs pass 476
checks with 96 kernel waiter workers, plus real user-thread interruption and
ordinary-signal killable-wait controls. This supplies driver synchronization
semantics while retaining original algorithms; it does not establish GPU fence
completion or reduce a measured time-to-Mesa estimate. Native whole GEM/ring
compilation still exposes later compatibility dependencies.


## Native preemption/helper integration result

The port retains original Linux helper algorithms and supplies real native
preemption/IRQ adapters shared with spin/rw locks. Physical RenkuOS compilation
and twelve final diagnostic runs pass 3436 checks, with actual unload and 48,000
VMA stress operations. A missed-overlap setup failure is preserved and corrected
by requiring actual reader overlap before stress. Host sanitizers and compiler/
mutation controls complement the native results. This is compatibility progress;
full original GEM/ring builds still fail at later headers and no GPU workload or
performance comparison exists. See physical-preempt-001 and preempt-host-001.

## Original bitmap/search reuse result

Complete original Linux bitmap-search and population-count implementations now
run on RenkuOS, replacing further local helper equivalents with upstream code.
Four native46,389-check runs pass against an independent oracle, including prior
completion/preemption cases; VMA/KPI regressions and clean unload also pass.
Compiler warnings and later whole-file failures remain explicit. This reduces
compatibility dependencies without establishing a full i915 build, GPU workload
or a performance comparison. See physical-bitmap-001 and bitmap-host-001.

## Original CPU-mask reuse result

Original Linux CPU-mask helpers now operate on actual native CPU/enabled state
with a fixed-topology lifecycle. Four native 46555-check runs pass, including
previous search/completion/preemption coverage; VMA/KPI regressions unload cleanly.
This removes an accidental single-CPU fallback and advances full-source compilation
to later VM and raw-lock integration. It supplies neither a replacement native
GPU driver nor GPU workloads/performance data. See physical-cpumask-001.

## Original raw-lock and bit-spin reuse result

Original Linux lock structures and complete bit-spin algorithms now operate over
native OS primitives. Four46786-check native runs include two bit workers each;
32000 protected updates and VMA/KPI regressions pass with actual unload. The native
input outage preceded deployment and was recovered by the user's restart. This
is tested compatibility progress, not a GPU workload/performance result or full
i915 build. Later VM/service failures remain preserved in physical-raw-lock-001.


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


## 2026-11-14: sequence-lock checkpoint

Complete original seqcount/seqlock algorithms now pass actual native tests:
four47087-check completion runs plus four650-check VMA runs total190948,
zero failures and actual unload. Eight sequence workers exercise16000 writes,
16000 accepted snapshots and four required stale-read rejections. Host sanitizer,
always-accept retry mutation and configuration/link controls pass. All113 original
imports remain unchanged. Supported scope is non-RT/non-debug task readers,
externally serialized/non-preemptible short writers and ordinary RAM; BH methods
remain unresolved. See experimental/i915/evidence/physical-seqlock-001.

Native physical GEM now stops at asm/stat.h, with sequence type errors resolved.
Ring/GTT still need real process-mm/task integration in sched/mm.h. Linux page/
filesystem lifetime, full i915 compilation and GPU execution remain unfinished.


## 2026-11-14: owned-page VM bridge

Real native wired page runs, original Linux refcounts, balanced atomic mappings
and a joined asynchronous release worker now pass actual RenkuOS tests. Four
completion runs of47294 and four VMA runs of650 total191776 passing checks.
Native area queries prove20 allocations/deletions; eight page workers perform
2048 map/reference iterations. Host lifetime/failure/mutation and header controls
pass. All114 Linux originals remain unchanged. See experimental/i915/evidence/
physical-pages-001 and completion/README.md for contracts and unsupported modes.

This supplies managed kernel backing, not general Linux folios, reclaim,
page cache, global memmap or user-copy fixup semantics. Physical GEM now fails
on page-flags/folio metadata and VM bounds through shmem; ring/GTT still need
real process-mm/task integration. Full original i915 build and GPU execution
remain unfinished; the full port objective stays active.
