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
  --linux-source /path/to/pinned-linux --output NEW_DIRECTORY
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
