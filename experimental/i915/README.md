<!-- SPDX-License-Identifier: MIT -->
# Bounded Linux i915 compatibility experiment

No GPU execution has been demonstrated. The current port compiles and runs
complete original Linux completion/swait, DRM VMA and drm_mm/rbtree modules
through native OS adapters, plus bounded ring/emitter slices. Linux DMA adapters
use existing native bus_dma. Shared IRQ/preemption integration and original
compiler/arithmetic/endian helpers pass physical N100 regressions; the manifest
verifies 87 unchanged upstream files. Complete original GEM/ring builds still
fail at later dependencies. See evidence/physical-preempt-001 for current native
results and compiler errors. Original bitops/bitmap-search/weight helpers now
pass native oracle tests too; see evidence/physical-bitmap-001 for the newest
results and later compiler errors. The recipes and checkpoint notes below record the
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
