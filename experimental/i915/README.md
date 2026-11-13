<!-- SPDX-License-Identifier: MIT -->
# Bounded Linux i915 compatibility experiment

Latest development: evidence/store-host-001 records49504 passing host sanitizer
checks for persistent backing, balanced pins and concurrent canonical folios.
The native bridge uses pageable anonymous areas and real VM wire/unwire APIs;
native compilation/runtime validation of this addition is pending USB recovery.

Previous development: evidence/folio-host-001 records47416 passing host sanitizer
checks for managed folio heads, original local maps and batch reference release.
Native validation of these additions is pending USB input recovery. All118
original imports are unchanged. Full original GEM shmem is now included in the
whole-source compiler loop; it still fails at unported VM dependencies.

Last native checkpoint: evidence/physical-pages-001 records191776 passing native checks,
including real owned page allocation/references, original atomic mapping helpers,
joined release-worker teardown and VMA regressions. All114 original imports
remain unchanged. Full original i915 builds still fail at VM/folio/process services.

No GPU execution has been demonstrated. The current port compiles and runs
complete original Linux completion/swait, DRM VMA and drm_mm/rbtree modules
through native OS adapters, plus bounded ring/emitter slices. Linux DMA adapters
use existing native bus_dma. Shared IRQ/preemption integration and original
compiler/arithmetic/endian helpers pass physical N100 regressions; the manifest
verifies 118 unchanged upstream files. Complete original GEM/ring builds still
fail at later dependencies. See evidence/physical-preempt-001 for current native
results and compiler errors. Original bitops/bitmap-search/weight helpers now
pass native oracle tests too; see evidence/physical-bitmap-001 for the newest
results and later compiler errors. CPU-mask/architecture integration now passes
native tests as well; evidence/physical-cpumask-001 records the latest boundary.
Original lock structures/bit-spin algorithms now pass native tests too; see
evidence/physical-raw-lock-001 for the latest results and remaining VM failures.
The latest scoped RCU/logging/device-ID results are in evidence/physical-rcu-001.
The recipes and checkpoint notes below record the
incremental development; current limits are in the root PORT_BLOCKERS.md.

Prepare the target headers from the GPU project's RenkuOS baseline:

```sh
git clone --filter=blob:none --no-checkout https://github.com/RenkuOS/Source.git renku-source
git -C renku-source sparse-checkout init --cone
git -C renku-source sparse-checkout set headers
git -C renku-source checkout --detach 4799bb5a188c4bb6bbef967287640cc21a061a0d
make target-objects RENKU_SOURCE="$PWD/renku-source"
```

Run from this directory. Source preparation needs network access. The build uses
GCC's builtin headers and the real RenkuOS headers with `-nostdinc`, `-mno-red-zone`
and `-mgeneral-regs-only`. Fedora GCC emits freestanding x86-64 objects; the OS
target toolchain, native kernel link/load and runtime ABI remain unvalidated.
The partial link output is `build/target/i915_components.o`, with per-object
commands/hashes in `build/target/manifest.json` and compiler dependency files.
Upstream imports are hash-verified before this build.

```sh
make allocator-test
```

This runs the actual upstream `drm_mm` allocator and rbtree under ASan and UBSan.
It requires the compiler's matching sanitizer libraries. On a host using locally
extracted libraries, pass `SANITIZER_LDFLAGS='-L/path/to/libs -Wl,-rpath,/path/to/libs'`.
Evidence 019 records the exact command used here. Leak detection and sanitizer
checks were not disabled. The test does not simulate a GPU or prove DMA coherency.

The include wrappers provide no page-allocation or workqueue service. The
complete release allocator needs neither. Reached refcount APIs have real native
semaphore/spinlock adapters, compiled but not executed in the native kernel.
Its caller must serialize modifications. Debug stack-depot/list backends are
excluded explicitly, and enabling an unsupported configuration must fail.

Linux errno values remain internal. A future native entry point must translate
them to `status_t`; passing Linux negative errno directly to native userspace
would be incorrect. The private ring/request layouts are not Linux object ABI.

The branch preserves existing display files. No module was installed, device
claimed, hardware register touched, or `gpu_exec_probe` success fabricated.
See root progress/dependency/blocker/comparison files for remaining work.

Reference-count and native atomic checks:

```sh
make refcount-test SANITIZER_LDFLAGS='<matching sanitizer library flags if needed>'
make target-atomic-test RENKU_SOURCE=/path/to/pinned/renku-source
```

The first uses the unchanged Linux kref/refcount algorithms and genuine host
pthread locks under ASan/UBSan; host IRQ operations abort as unsupported. The
second executes a kernel-call-free object compiled with real RenkuOS headers
and RENKU_NATIVE on the x86-64 host. Neither is a native OS runtime test.
Linux's required `-fno-strict-overflow` flag preserves saturation arithmetic;
evidence 027 retains the failure without that flag. Sanitizers remain enabled.

To continue the full-file attempt, prepare a Linux v6.6 checkout at
`ffc253263a1375a65fa6c9f62a893e9767fbebfa` containing `include/` and
`drivers/gpu/drm/i915/`, then run after target-objects:

```sh
python3 whole-ring-attempt.py --linux-source /path/to/linux --log /path/to/new-log
```

This command intentionally returns the actual compiler failure, currently at
wound/wait task/locking dependencies and dma-fence errno definitions. The
PREEMPT_RT branch is excluded explicitly; no RT mutex interface is simulated.

Wound/wait prerequisites now include caller-owned task registration, 64-bit
stamp increments and named mutex held-state. Register a task record before
original code uses `current`; retain it until every referencing acquire context
has finished, then unregister it on the owning thread. No scheduler, task
reference API, wait/wakeup or full wound/wait locking implementation is supplied.

```sh
make ww-context-test SANITIZER_LDFLAGS='<matching sanitizer library flags if needed>'
```

This runs original Linux context initialization with real host thread identity
and mutexes. It tests prerequisites only. `build/target/ww_mutex.o` is compiled
from unchanged ww_mutex.h with `-x c -fkeep-inline-functions`; unresolved
ww_mutex_lock methods are retained in the partial-link dependency inventory.

The native clock adapter calls system_time_nsecs. Error-pointer tests also check
that Linux errno installation preserves the native B_* status aliases. RCU
metadata and acquire pointer loads do not supply read-side lifetime protection,
grace periods or callback reclamation. PAGE_SHIFT is checked against native page
size; unmap_mapping_range is declared but unimplemented. The current whole-ring
failure is now in DRM VMA rwlock/VM declarations (evidence 046).

Original batch-start emitter integration:

```sh
make emitter-test SANITIZER_LDFLAGS='<matching sanitizer library flags if needed>'
make target-emitter-test RENKU_SOURCE=/path/to/pinned/renku-source
```

Both gen8_emit_bb_start functions are extracted unchanged and use the real
ported intel_ring_begin. Packet tests include ring wrap, guards and a batch
address above 4 GiB. The target-object test executes CPU packet construction;
its missing timeline/wait backends abort if called. Neither test submits work
or writes a GPU result. Ring/request layouts remain private slice interfaces,
with an explicit software nopreempt property; they cannot be mixed with full
i915 request objects. Full-file compilation still fails at the native rwlock/
completion/IDR/VMA boundary (evidence 062).


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


Recovery update2026-11-14: the revised VMA fixture is now native validated,
four650-check runs with zero errors, two readers and actual unload. Rebuilt
completion passes four47056 checks; combined190824. The pending status above
is historical. Evidence: experimental/i915/evidence/physical-lockref-recovered-001
(relative to repository root). Whole original i915 compilation still fails.
