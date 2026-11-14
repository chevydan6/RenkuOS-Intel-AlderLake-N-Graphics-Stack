<!-- SPDX-License-Identifier: MIT -->
# Original Linux completion and simple wait queues

The complete Linux v6.6 `kernel/sched/completion.c` and `swait.c` algorithms,
including their public headers, are byte-exact imports. A forced include supplies
the surrounding declarations normally supplied by Linux's sched/build_utility.c.
Native event, task-scope and deadline adapters connect them to existing RenkuOS
semaphores, IRQ spinlocks and monotonic time. No GPU algorithm is replaced.

This supplies counted completion, complete_all/reinitialization, blocking,
timeouts, interruptible and killable waits for registered native entry scopes.
A wake before OS blocking is retained by the semaphore; stale wake credits are
drained before publishing the next wait. Original queue code removes a waiter
when waking it, preserving distinct wakeups for consecutive completions.
The port's tick unit is explicitly 1ms (HZ=1000).

The actual N100 test compiles and loads these complete original implementations.
It checks pre-signaling, timeout/no-token behavior, two queued waiters, counted
wakeups, broadcast/reuse, IRQ-disabled complete/complete_all and scope cleanup.
User-thread SIGUSR1 interrupts an interruptible ioctl wait with Linux -ERESTARTSYS;
a killable wait stays blocked through that ordinary signal and finishes on a
completion. Killing a waiter was not tested. No GPU interrupt or GPU work occurs.

Scope limitations remain explicit:

- Task records are caller-owned entry scopes, not general Linux scheduler tasks
  or reference-counted task handles. Waiters must finish before unregistering;
  the callback releases their native event before module unload.
- First blocking entry creates a semaphore in sleepable context; creation failure
  is currently fatal, as are invalid context/lifecycle operations. Production
  entry/error handling still needs integration before this is a full driver.
- Four task states are supported: running, uninterruptible, interruptible and
  killable. Native acquisition checks pending signals; the adapter records an
  interruption for that scope/wait mode. It is not a general signal subsystem.
- Wake CPU-placement flags are advisory and native scheduling chooses placement.
  IO waits use the same real native blocking path without Linux blkio accounting.
- Host tests use real pthread/POSIX synchronization and model IRQ state. Host
  kill-only signal behavior is not claimed; the native backend uses the verified
  B_KILL_CAN_INTERRUPT service. General Linux wait.h methods remain unresolved
  unless supplied by the imported swait/completion code.

The same checkpoint adds original x86-64 UAPI scalar types, generic do_div,
kconfig helpers and lockdep-off types, plus real atomic bit operations/raw IRQ
locking. Instrumented lockdep and PREEMPT_RT builds fail explicitly.

## Native preemption and shared IRQ scopes

Native spinlocks, reader/writer locks and preemption exclusion share one wired
per-CPU scope table. Initialize it once per diagnostic/driver before using these
interfaces; exclude new users before shutdown. CPU topology must remain fixed.
Shutdown rejects live scopes. The old rwlock/preemption lifecycle names delegate
to this same backend and must not be initialized separately.

`preempt_disable` uses actual native IRQ masking. This is stronger exclusion
than Linux task preemption alone: sections must remain short and nonblocking.
Nested IRQ requests are recorded separately from physical masking, so an inner
`spin_unlock_irq` cannot enable physical IRQs inside an outer preemption or read
lock. IRQ-save flags are opaque restore tokens. Scope count includes these
adapted locks; Linux hardirq/softirq/NMI counter encodings are not synthesized.
Interrupt-context classification remains unresolved pending real IRQ entry
integration. There is no simulated Linux scheduler behind these interfaces.

Original Linux compiler attributes, build_bug, minmax, alignment, arithmetic and
byteorder helpers replace conflicting local spellings. Native libc declarations
are loaded before the Linux namespace. These remain byte-exact upstream imports.

The physical continuation passes four 149-check completion/preemption runs,
four 648-check VMA runs and four 62-check older kernel-service runs. It covers
nested and overlapping lock/preemption lifetimes, explicit IRQ requests, active
scope teardown rejection and original helper semantics. See
../evidence/physical-preempt-001 for results, earlier failed attempts and limits.

The additional host model compiles the actual native adapter sources against
the target headers with ASan/UBSan. Its serialized two-CPU native-service model
checks state transitions and rejects a premature-IRQ-enable mutation; it does
not demonstrate hardware concurrency:

```sh
python3 experimental/i915/tests/run-irq-scope-model.py --output NEW_DIRECTORY
```

Host sanitizer test and a lost-count mutation control:

```sh
python3 experimental/i915/completion/run-host-tests.py --output NEW_DIRECTORY
```

Native reproducible bundle (the Linux checkout must contain its include and
i915 source trees; every exported regular reference file is checked against the
pinned Git blob before copying):

```sh
python3 experimental/i915/completion/prepare-bundle.py \
  --linux-source /path/to/pinned-linux --renku-source /path/to/pinned-renku --output NEW_DIRECTORY
# Transfer/extract payload.tar.gz; in actual RenkuOS Terminal:
sha256sum -c payload.sha256
bash completion/build-native.sh > build-native.log 2>&1
bash completion/run-native-tests.sh
bash vma/build-native.sh > vma-build.log 2>&1
bash vma/run-native-tests.sh
bash vm/build-native.sh > kpi-build.log 2>&1
bash completion/run-kpi-regression.sh
bash completion/attempt-whole-native.sh
```

The final command preserves real compiler failures for the complete original GEM
and ring files. They still fail; successful completion-library tests do not mean
the full i915 driver compiles. See ../evidence/physical-preempt-001.

## Original bit operations and bitmap searches

Complete original Linux `lib/find_bit.c`, `lib/hweight.c`, `linux/bitops.h`,
`linux/find.h` and `linux/bitmap.h` now supply bitmap-search algorithms and
non-atomic helpers. The import manifest verifies 87 unique files. Generic Linux
bit scans, constant weights and non-atomic mutations replace local equivalents;
the architecture adapter uses compiler atomics for shared-word mutations, with
relaxed non-returning operations, fully ordered test-and-modify, acquire lock
acquisition and release unlock. The generic x86-64 Linux ioctl encoding is also
imported; native ioctl dispatch is still a separate integration requirement.

The search test compares original first/next/last/nth, zero, AND, AND-NOT and OR
searches with an independent bit-by-bit oracle. It covers sizes 0 through 192,
word edges, dirty final words, no matches, offsets beyond the limit and inline
constant-size paths. Absent nth results are interpreted as >=size. Linux v6.6 can return 64
for a missing nth bit in a constant partial word, despite its comment saying
size; the port preserves that behavior and does not promise exact-size absence. Empty public searches are checked with NULL
backing where they must not dereference it. All 192 bit indices exercise atomic
and non-atomic mutations; this is not an atomic concurrency stress test.

Four final N100 runs pass 46,389 checks each, including the previous completion/
preemption tests. VMA and older KPI regressions also pass, with actual diagnostic
unload. Host ASan/UBSan passes 46,362 checks. A mutation ignoring the first-word
search offset aborts as required; invalid GENMASK input still fails compilation.
The additional first-header checks are reproducible after target-objects:

```sh
python3 experimental/i915/tests/run-bitops-headers.py --output NEW_DIRECTORY
```

Only the complete search/weight library and used inline bitmap functions are
linked. External bitmap allocation/parsing/other `lib/bitmap.c` services remain
unimplemented, and whole i915 translation units still fail at later dependencies.
This does not provide native CPU-mask semantics or GPU execution.

Compiler diagnostics remain explicit. The original find header's constant
GENMASK(size-1,0) input checks trigger GCC -Wextra unsigned-zero-comparison
warnings; that warning is scoped off around that imported header. Invalid-mask
compile errors remain enabled and tested. The original private nth helpers can
read uninitialized tmp if called directly with size 0. Their public find_nth
wrappers guard n>=size before calling them. Native and host builds retain
-Wmaybe-uninitialized warnings for this original source but downgrade that one
warning from Werror for find_bit.c only; callers must use the public guarded
APIs. The tests exercise public zero-size and nonzero-size bounds, not unsafe
private zero-size entry. No imported algorithm is modified. Raw diagnostics,
exact source/binary hashes and final results: ../evidence/physical-bitmap-001
and ../evidence/bitmap-host-001.

## Original CPU masks and architecture configuration

The port now selects SMP and a 64-CPU mask capacity, matching the supported
native EFI/x86-64 baseline's SMP_MAX_CPUS. The complete original cpumask.h,
threads/numa, cache/linkage, limits and parameter headers are byte-exact imports;
the manifest verifies 99 files. The Linux mask layout remains separate from
native CPUSet. Native smp_get_num_cpus and get_cpu_info supply actual dense CPU
identities and enabled state. Present/possible masks contain discovered CPUs;
online/active masks contain enabled CPUs, and dying is empty for this fixed
snapshot. Startup rejects empty, oversized or invalid state and propagates query
failure. Callers serialize initialization/reset, exclude new users during teardown
and keep topology/enabled state fixed for the module lifetime. Live CPU hotplug,
enable/disable notifications, affinity, cross-CPU calls and NUMA placement still
need integration. The snapshot is not a general native scheduler replacement.

Original inline cpumask variables own their storage with OFFSTACK disabled;
their original no-allocation alloc/free methods are retained. Constant one-bit
and all-bit tables supply original mask lookups. Other external cpumask/bitmap
helpers remain unresolved unless supplied by the existing search library.
The removed native num_online_cpus macro no longer conflicts with Linux's UP
fallback; the original SMP accessor reads a native-backed count. Atomic types
are visible before operation headers, with actual raw-read/CAS/decrement helpers.

Original x86 cache headers define the selected 64-byte alignment, checked against
CPUID on the N100. These are alignment helpers, not cache-maintenance services.
Linux limits and param wrappers select Linux macro spellings after native libc
and kernel declarations. They do not implement Linux VFS or native time-accounting
ABIs. HZ stays 1000 and USER_HZ 100. A bootstrap guard keeps kernel typedefs and
cast-valued limits out of glibc's initial limits.h parsing. First and repeated
inclusion checks cover 44 host/target combinations.

The diagnostic report is now version 2 and records CPU count, online count and
mask. Actual RenkuOS runs report 4 CPUs, 4 online, mask 0xf, with four 46,555-check
runs passing. Existing VMA/KPI regressions and all host sanitizer/negative controls
pass; actual diagnostic teardown is observed. The CPU adapter model tests sparse
enabled IDs, CPU 63, limits, failed queries, changed count, reinitialization and
reset under ASan/UBSan with real target declarations. Admitting disabled CPUs
must fail its mutation control. Reproduce after target-objects:

```sh
python3 experimental/i915/tests/run-cpumask-model.py --output NEW_DIRECTORY
```

Whole original ring compilation now reaches generated/bounds.h through Linux VM
page-layout definitions; GTT reaches raw architecture spinlock types; physical
GEM retains device-number/RCU/bit-unlock and VM-layout dependencies. No synthetic
Linux page layout or scheduler was added to hide those errors. See
../evidence/physical-cpumask-001 and ../evidence/cpumask-host-001. Existing original
find_bit warnings and private nth-search preconditions described above still apply.

## Original raw lock structures and bit-spin algorithms

Linux's original raw_spinlock_t, spinlock_t and rwlock_t structures and static
initializers now wrap native architecture storage. Raw and regular locks are
distinct C types; regular methods select their original rlock member, and native
raw/RW backends use the original raw_lock member. The native saved-IRQ field was
removed from individual locks; the already tested shared CPU scope manager owns
that state. Host architecture storage uses actual pthread locks. RT, debug-spin
and lockdep instrumentation remain unsupported rather than successful no-ops.

Complete original bit_spinlock.h algorithms now run over native preemption and
compiler atomic bit methods. __clear_bit_unlock provides the weaker release-store
operation; all flags in the word must share that bit lock, as upstream requires.
Actual native cases cover failed trylock restoring its nested scope, both unlock
forms preserving flags, nested preemption and original raw/regular/RW initializers.
Two kernel workers start through an original completion gate and perform 8000
protected updates per run, checking value/mirror consistency. Their lifetimes
overlap; no contention counter or exhaustive scheduling claim is made.

A linker failure after the layout change exposed __bitmap_equal, previously
optimized out of constant-size calls. The full original lib/bitmap.c is imported;
extract-bitmap.py verifies its hash and extracts only the complete, unchanged
__bitmap_equal function with a function hash. Its generated C/metadata are
exported in native bundles. Empty, partial/multiword, dirty-tail and differing
bit cases compare with an independent oracle; an ignored-tail-mask mutation
must abort. This supplies that helper, not the complete bitmap library or its
allocation/parsing services. All 104 imports remain byte-exact.

Four actual N100 runs pass 46786 checks each, with eight total bit workers,
32000 bit-lock updates and 96 prior waiter workers. Native VMA/KPI regressions
and actual diagnostic unload also pass. The report is version3 with bit-worker
metrics; its matching module/client are built together. Host ASan/UBSan passes
46742 checks plus mutation controls; 54 header-order controls and 19 extra
native-source object probes pass separately. Those host probes do not replace
actual native compilation/runtime evidence. See ../evidence/physical-raw-lock-001
and ../evidence/raw-lock-host-001. Previous find_bit warnings remain unchanged.

The original complete GEM/ring compiler attempts still fail on RenkuOS. Raw-lock
type and bit-unlock errors are resolved; later failures include Linux VM bounds/
page layout, logging/task-name declarations and physical-GEM RCU/device-number
integration. No invented bounds or page structures were used to hide errors.


## 2026-11-14: scoped RCU, logging and device IDs

A synchronous RCU bridge now uses actual native reader/writer spin exclusion
and the shared IRQ scope manager. Outer read entry requires IRQ-enabled,
preemptible task context outside other atomic/spin scopes. Nested reads skip
lock reacquisition, including while a writer waits. Readers must be short and
nonblocking; topology is fixed. synchronize_rcu acquires native writer exclusion
and waits for preexisting readers with IRQs disabled. This stronger exclusion
can also block new readers; it does not provide general Linux nonblocking RCU
or bounded grace latency under continuous readers. Init/uninit are serialized;
the owner must exclude new and running users before shutdown. Depth checks do
not supply unload references. Callback queues, call_rcu, rcu_barrier, encoded
frees, IRQ/NMI/BH readers and expedited grace periods remain unsupported.
Unimplemented callback symbols have a private prefix to prevent accidental
binding to another kernel implementation.

The printk bridge emits through native dvprintf with a 512-byte formatting
buffer. Common C integer/string/character/plain-pointer formats and levels0–7
are supported. Unsupported prefixes (including KERN_CONT), %pV and other Linux
pointer extensions, %n and floating formats return -EINVAL with a diagnostic;
truncation returns -ENOSPC. CONFIG_PRINTK selects real device-printk declarations;
their implementations remain unresolved. Registered task comm is a real native
thread-name snapshot (16 bytes), queried before taking the registry lock. It is
not a complete Linux task ABI. Original kernel device-number helpers operate on
unsigned Linux IDs, with a scoped dev_t rename; native filesystem IDs retain
their own type and are not mapped by this adapter. The UAPI include selects its
kernel branch locally without globally enabling Linux __KERNEL__.

All 107 original imports remain unchanged. On the physical N100, native GCC13.3
and SDK518 built the final diagnostic module/client. Four runs pass 46934 checks
each, including eight RCU workers and four real heap frees after grace periods.
The test verifies a waiting writer, a nested reader behind it, unlinking and
safe reclamation. Four VMA/KPI regression runs bring the total to 190576 checks,
zero failures, with actual module unload. Four native logging markers were
observed. Host ASan/UBSan passes 46879 checks; the separate target-header RCU
model rejects omitted grace waits and two forbidden context entries. There are
64 header-order controls and eight additional native-source object probes.

The complete original ring, GEM GTT and physical-GEM native compiler attempts
now all stop at generated/bounds.h through Linux VM/page-layout dependencies.
Earlier RCU-read, logging/task-name and device-number errors are resolved at
this compiler boundary. No page layout or bounds were fabricated. Full i915
compilation and GPU execution remain unproven. See evidence/physical-rcu-001
and evidence/rcu-host-001 (relative to the experiment root).


## 2026-11-14: original lockref and GFP helpers

All111 original imports verify against the pinned v6.6 Git blobs. Complete
lockref.c/.h now use their original spinlock fallback over native locks. Native
B_SPINLOCK_IS_LOCKED supplies the real held-state query; it is not an ownership
query. Host tests query actual pthread mutexes. The original fallback deliberately
returns -1 from lockref_put_return without decrementing, allowing caller retry.
Dead, zero, one and positive reference transitions and put_or_lock returning a
held lock are tested, with native IRQ/scope restoration checks.

The bounds include shim supplies no guessed SPINLOCK_SIZE, NR_PAGEFLAGS,
MAX_NR_ZONES or page structures. It accepts only the original lockref header
with architecture cmpxchg disabled, whose preprocessor branch needs no bounds.
Every VM inclusion outside that scope and enabling cmpxchg are compile errors,
including an inclusion after lockref has already been included.

The full original gfp_types.h replaces copied flag constants. extract-gfp.py
hash-checks the full original gfp.h and emits its complete unchanged
blocking predicate, with a function hash. Native kmalloc uses that predicate;
its supported flags and task-context restrictions remain unchanged. Page
allocation, Linux zones/reclaim and process-mm lifetime remain unresolved.
Native bundles now carry the extracted header and metadata.

Four actual N100 native GCC13.3/SDK518 completion runs pass47056 checks each
(188224 total, zero failures), including eight lockref workers and32000
iterations; the diagnostic actually unloads. Host ASan/UBSan passes46987 checks
with concurrent lockref workers; dead-reference and unlocked-mark-dead controls
are rejected. Seventy header-order cases and four bounds/cmpxchg rejection cases
pass. Host VMA/DMA regressions and target-header compilation pass.

The old native VMA rendezvous failed on its third run (max_readers1,
worker_errors1, four assertions). Evidence retains that failure. A revised
fixture uses real synchronous CPU callbacks on two enabled CPUs, preserving
the overlap and100ms bound. It passes target-header compilation; native retest
is pending because USB input timed out and the next source extraction stalled
before the revised build began. No passing native VMA regression is claimed
for this checkpoint. See ../evidence/physical-lockref-001 and ../evidence/lockref-host-001.

Cross-source whole-file attempts advance past the prior bounds includes:
ring/GTT now fail in Linux sched/mm.h on missing real process-mm/task integration;
physical GEM fails on seqcount/seqlock types and asm/stat.h. These are cross
compiler results, not new native whole-file attempts. Full i915 compilation
and GPU execution remain unproven.


Recovery update2026-11-14: the revised VMA fixture is now native validated,
four650-check runs with zero errors, two readers and actual unload. Rebuilt
completion passes four47056 checks; combined190824. The pending status above
is historical. Evidence: experimental/i915/evidence/physical-lockref-recovered-001
(relative to repository root). Whole original i915 compilation still fails.


## 2026-11-14: original sequence counters and locks

Complete Linux v6.6 seqlock.h and configuration-disabled kcsan-checks.h are now
unchanged imports (113 total). Native ordinary-RAM read/write fences, regular
IRQ spinlock wrappers and the existing preemption scope support their original
algorithms. cpu_relax is shared in asm/processor.h. The preemptible adapter is
function-like, preserving original seqcount generic property names. No sequence
algorithm was replaced. Compiler barriers and x86-64 RAM ordering do not supply
MMIO ordering or i915 DMA/cache-domain transitions.

Supported/testing scope is non-RT, non-debug, task-context seqcount/seqlock
operations, ordinary RAM snapshots and short nonblocking writer sections.
Raw seqcount writers must be externally serialized and non-preemptible; native
spin/preempt scopes provide stronger IRQ exclusion. A reader must not interrupt
its own writer or follow reclaimable pointers. Plain, spin-associated and
mutex-associated counters, writer IRQ variants, exclusive readers and counter
wraparound are tested. Latch/NMI/IRQ/softirq integration is not validated by this
checkpoint. BH methods remain uniquely unresolved, and their actual link failure
is tested. KCSAN/RT/debug instrumentation remains unsupported rather than enabled
with fake runtime services. The imported disabled instrumentation branch is used.

Actual native GCC13.3/SDK518 N100 compilation, four47087-check completion runs
and four650-check VMA regressions pass:190948 checks, zero failures and actual
unload. Eight sequence workers perform16000 writes/16000 accepted stress
snapshots; four required stale-read rejections prove retry behavior independent
of scheduling. Native mutex-associated preemption and IRQ restoration pass.
Host ASan/UBSan47011 checks pass, and an always-accept retry mutation is rejected.
There are80 header-order controls, eight unsupported-configuration failures and
two unresolved-BH link controls. Native source checks match7083 entries before
and after tests, including6783 reused byte-identical reference files.

Native physical GEM now advances past sequence-lock declarations and stops at
asm/stat.h. Ring/GTT still fail on real process-mm/task integration in sched/mm.h.
No full i915 build or GPU execution is claimed. Evidence is in
experimental/i915/evidence/physical-seqlock-001 and seqlock-host-001, relative to
the repository root. Temporary SSH is stopped, lease released, media preserved.


## 2026-11-14: native owned pages and atomic mappings

Linux alloc_pages now owns real native wired backing through create_area_etc,
using SDK virtual/physical restriction structures and verified alignment and
physical extents. Order0 and explicitly compound higher-order runs are supported
for GFP_KERNEL, optional __GFP_ZERO/__GFP_COMP. Other zones, atomic reserves,
ordinary higher-order splitting, reclaim and migration are not implemented;
unsupported modes fail allocation. Backing stays wired for its entire lifetime.
The existing two-field page view remains the descriptor ABI; a separate owner
registry holds the actual area and original Linux saturating refcount. Tail
references resolve to the compound head. virt_to_page/nth_page operate only on
registered managed runs; they do not fabricate a global Linux physical memmap.
Borrowed DMA views remain caller-owned and cannot acquire managed references.
Lookup is currently linear in the number of live runs.

Last put_page removes the run from lookups and queues release without sleeping,
including under atomic exclusion. A dedicated native worker deletes the actual
area and metadata. Drain accounts for pending work; shutdown rejects live or
pending runs, then wakes and JOINS the worker before destroying events or
allowing module unload. Lifecycle is serialized and the module owner must
exclude new/running callers during shutdown. Borrowed lookup results require
the caller to retain backing ownership; DMA mappings do not gain implicit pins.

Complete original non-highmem kmap_atomic/__kunmap_atomic and its type-checking
macro are extracted unchanged with hashes from the114th original import.
Registered task scopes track balanced fault nesting and cannot unregister with
an outstanding fault scope. The bridge calls real native forbid/permit_page_faults
and existing preempt exclusion. Native fault prohibition is not Linux user-copy
exception-table/fixup behavior; this profile uses valid wired RAM only. User-copy
methods remain uniquely unresolved. Highmem, local mapping slots, RT, cache-alias
flush profiles and general IRQ/NMI mapping entry are not supported by this bridge.
This is not a complete folio/page-cache or Linux VM layout implementation.

The native C++ bridge requires installed SDK private headers plus original build
configuration headers, exported unchanged from the pinned native source (with
License.md and provenance). prepare-bundle.py now also requires --renku-source.
Actual SDK vm.h/vm_types.h hashes match the pinned headers; structures are not
re-created. All114 Linux imports remain unchanged. The native-only missing-area
test now uses actual B_BAD_TEAM_ID/B_BAD_VALUE results; early failed builds are
retained along with SDK include-path failures and the final passing build.

Four actual N100 GCC/G++13.3/SDK518 completion runs pass47294 checks each; four
VMA runs pass650 each, totaling191776 with zero failures and actual unload.
Twenty area runs were allocated/deleted, eight page-user workers perform2048
reference/map iterations, and actual native area queries prove final release.
Original scatterlist helpers verify offsets and physical IDs on managed runs.
Host ASan/UBSan47231 checks pass; omitted reference release/native deletion and
prior mutations are rejected. Ninety header-order cases,14 configuration/type
rejections, two unresolved-BH link controls and host DMA regressions pass.
All7102 payload entries verify before/after native testing, including shared
unchanged references; final runtime sources match bundle005. Evidence is in
experimental/i915/evidence/physical-pages-001 and pages-host-001 (repository root).

Whole native original builds still fail: physical GEM now reaches real page-
flags/folio metadata and generated VM bounds through shmem; ring/GTT retain
process-mm/task integration errors. No page flags, VM bounds or process objects
were fabricated. Full i915 compilation and GPU execution remain unproven, and
the overall port goal remains active. Temporary SSH is stopped and lease released.

## Managed folio heads and local maps (native validation pending)

Managed wired runs now expose a canonical folio view of the head descriptor.
The registry supplies actual order, page count, shape and original refcount
state; there is no Linux raw page-flags layout or global PFN memmap. Order-zero
allocations remain noncompound even when requested through folio_alloc. Tail
pages resolve to the same head; folio APIs require that canonical head. Callers
must retain an owning reference while using a borrowed page/folio lookup.

Complete original folio size/shift/PFN/page-selection helpers and the const
page_folio macro are extracted with provenance hashes. Complete original
non-highmem kmap_local_page/kmap_local_folio/kunmap_local helpers use backing
that remains mapped and wired until the owning reference is released. This
direct-map profile needs no per-thread slots and does not disable page faults
or preemption. Highmem, cache aliases and CONFIG_KMAP_LOCAL remain unsupported.

The complete original pagevec header supplies batch construction. Its release
backend consumes actual managed-folio references, including final references
under preemption exclusion; the joined native release worker owns deletion.
These wired pages never enter Linux per-CPU LRU queues. This is not a page-cache
provider: exceptional swap/DAX batch handling remains uniquely unresolved, as
do dirty/accessed/reclaim/writeback and unevictable mapping operations.

Host ASan/UBSan passes47416 checks, including concurrent local/atomic maps and
balanced repeated folio references. Tail-head redirection and omitted batch
reference release are rejected by mutation controls. Header tests pass94 include
combinations,18 expected configuration/type failures and four unresolved links.
The native test C units compile against the pinned Renku headers, but these new
runtime paths have not run on RenkuOS: KVM keyboard writes time out and custom
SSH is stopped. Evidence is in ../evidence/folio-host-001. Report/client version8
expects seven owned allocations/frees per completion run. The previous native
version7 checkpoint remains the last actual hardware result.

The unchanged complete i915_gem_shmem.c is now imported and included alongside
GTT, physical GEM and ring in the native whole-source compiler loop. Its current
cross-source failure reaches shmem_fs.h/swap.h/mmzone.h, generated VM bounds and
asm/page.h. Actual shmem backing, file/mapping lifetimes, page-cache pinning,
dirty/writeback/truncate behavior and process-mm integration are still required.

## Pageable backing stores (native validation pending)

The internal renku_linux_page_store API supplies the backing needed before a
Linux shmem/file adapter can be connected. It creates a native B_NO_LOCK anonymous
area rather than keeping every buffer page permanently wired. Reads obtain a
writable single-page native pin with lock_memory_etc(B_SYSTEM_TEAM, ..., B_READ_DEVICE)
and verify its actual physical extent. A live folio reference keeps that pin;
final put queues a matching unlock_memory_etc through the existing release worker.
The area retains the contents after unpin until its owner destroys the store.
Physical identity is queried again on the next pin. Direct CPU access requires
a live folio reference, including throughout DMA map/use/unmap.

The registry merges concurrent pins of the same store/index into one canonical
folio. A racing redundant native pin is released before returning the winning
head. Store destruction refuses active folios, pending unpins and in-flight pin
operations. Failed native area deletion republishes the owner for retry; empty
stores still prevent backend shutdown. The module owner serializes lifecycle
and excludes new callers during destruction/unload. Lookups remain linear.

Native source inspection at the pinned Renku revision confirms B_NO_LOCK uses
an anonymous cache, faults pages as needed and can use the VM's swap support.
The wire API retains the address space and pins both area ranges and physical
pages; unlock must use exactly matching parameters. These source facts do not
prove native behavior of this new bridge. Native C++ compilation, export/link
checks, VM paging/swap behavior and runtime teardown are still pending. The
host C++ probe cannot resolve the target SDK's <new> header and is retained as
a failed cross-compilation attempt, not a passing bridge build.

Host ASan/UBSan passes49504 checks with production C ownership logic. The model
uses real RAM and synchronization, explicit pin accounting and a changing
synthetic physical identity after complete unpin. It does not model native
faults, reclaim or swap. Tests retain data across unpin/re-pin, reject deletion
with live/in-flight/queued pins, balance simultaneous first pins, recover from
pin/area-deletion failures and use two workers for256 read/map/ref iterations.
Four additional mutations reject missing final/duplicate unpins, duplicate
canonical heads and live deletion. C adapters, native C tests and version9
client compile against pinned native headers. Report layout is112 bytes and
expects one store create/free and two workers/256 iterations per completion run.
See ../evidence/store-host-001 for exact results and remaining limits.

This internal backing provider is not yet a Linux shmem file/address_space
implementation. Size-changing truncate, dirty/accessed/writeback operations,
Linux GFP/reclaim constraints, userspace mappings and file/mapping lifetimes
remain unfinished. Complete original i915 files still do not compile, and no
GPU work has run. All118 unchanged Linux imports remain hash verified.


## Native shmem ownership checkpoint

The version10 provider adds real native-backed file/inode/address_space ownership
and shmem setup/read wrappers. Original Linux imports remain unchanged. Final file
close retains native backing and metadata through outstanding folios/in-flight pins;
the joined worker deletes the area and frees metadata after final unpin. Native
VM_NORESERVE uses actual overcommit/no-commit flags. Explicit store destruction
retains its synchronous unique-owner contract; final store put uses worker cleanup.

Focused ASan/UBSan lifetime tests and six mutations, 57729 integrated host checks,
100 header combinations and target-header C compilation pass. Four physical N100
runs of57786 checks plus four650-check VMA runs total233744 checks, zero failures
and actual diagnostic unload. The report is version10,128 bytes; each run expects
two shmem file creates/frees, two shmem workers and512 iterations. See
../evidence/physical-shmem-001 for exact native logs, source and binary hashes,
independent review and limitations.

Only ordinary sleepable GFP_KERNEL reads are supported. Other policies and atomic
contexts fail explicitly with EOPNOTSUPP. This does not satisfy the actual i915
HIGHUSER/reclaim/retry policy yet. Dirty/accessed, truncate/writeback, unevictability,
user mappings and full Linux file/cache services remain unresolved. Full original
GEM/ring builds and GPU execution are still unfinished.

Reproduce focused host validation:

```sh
python3 experimental/i915/tests/run-shmem-host-tests.py --output NEW_DIRECTORY
```


## Version11: native pinned-page state

The128-byte report now includes native-only dirty/accessed bridge checks. Actual
vm_page fields are inspected independently after clearing private diagnostic
backing. Tests verify prior values, invalid-input preservation and two gated,
joined native workers. The production primitive requires sleepable context and
a retained writable native pin; it is not a Linux atomic setter, writeback or
hardware-PTE synchronization implementation. Final native evidence and both
omitted-write controls are in ../evidence/physical-page-state-001.
