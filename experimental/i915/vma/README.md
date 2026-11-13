<!-- SPDX-License-Identifier: MIT -->
# Original DRM VMA offset manager

The complete Linux v6.6 `drm_vma_manager.c` and its header are imported byte for
byte. They run with the already imported original drm_mm/rbtree algorithms.
This is one of i915 GEM's DRM dependencies: it allocates page-based mapping
offsets, finds objects by offset, and tracks per-file access permission.
It does not map userspace pages or implement the broader GEM object lifecycle.

The OS adapters use RenkuOS's existing native reader/writer spinlocks. IRQ
exclusion is stronger than Linux's plain read_lock/write_lock, as required by
the native primitives. Shared IRQ restore requests and scope depth are per CPU,
allowing concurrent readers without sharing or racing on one saved-state field.
Startup allocates a wired scope table sized from the native CPU count; lock
initialization itself does not allocate. Module startup/shutdown must be
serialized and shutdown must exclude new users. The current CPU topology is
fixed for that lifetime. Shutdown rejects active scopes and frees the table.
Native spinlocks and preemption exclusion now use this same scope manager;
initialize it once per driver, not once per lock kind.

Sleeping `kmalloc` uses the native heap and rejects atomic/IRQ-disabled requests.
`kfree` uses native deferred_free when IRQs are disabled: upstream VMA revocation
frees an access record inside its write lock. That daemon frees ordinary native
heap memory and holds no callback into the unloadable adapter. General slab
caches and atomic allocations are not supplied.

Compiler compatibility now uses the original Linux compiler annotations and
log2 header, GCC bit scans, shared compiler stdarg definitions, allocation-type
propagation and the native x86-64 C calling convention. The imported helper
headers retain their licenses. No original i915/DRM algorithm was edited.

Run the host test:

```sh
python3 experimental/i915/vma/run-host-tests.py --output NEW_DIRECTORY
```

It uses ASan/UBSan and real pthread reader/writer locks, with a modeled heap
backend for allocation failure injection. A mutation that leaves revoked access
alive must abort. `SANITIZER_LDFLAGS` may supply matching local runtime paths.
Host locks do not validate native IRQ semantics.

Prepare an exclusive native bundle, transfer/extract its payload, and run in
the actual RenkuOS Terminal with its matching installed SDK:

```sh
python3 experimental/i915/vma/prepare-bundle.py --output NEW_BUNDLE_DIRECTORY
# On RenkuOS, from the extracted bundle:
sha256sum -c payload.sha256
bash vma/build-native.sh > build-native.log 2>&1
bash vma/run-native-tests.sh
```

The native test installs only `i915_linux_vma_test`, runs the complete VMA module
and three stress workers, then checks diagnostic unload and unchanged
graphics-device publication. It verifies range capacity/lookups, reference-counted
and allow-once access, revocation, original log2 boundaries, concurrent readers,
writer exclusion, nested IRQ restoration, zeroed allocation and IRQ/atomic
allocation rejection. It never touches GPU registers or submits a GPU command.

The previous two-reader kernel-thread rendezvous had a100ms IRQ-off deadline.
It passed earlier checkpoints but missed overlap on the third run of the
lockref continuation: max_readers1 and one rendezvous error produced four failed
assertions. The failure and original source hashes are retained in
../evidence/physical-lockref-001/vma-initial-failed.

The revised fixture uses exported call_all_cpus_sync and selects two enabled
CPUs before callbacks start. Each selected callback holds the real read lock
until both readers enter, with the same100ms IRQ-off bound and no semaphore
wait or allocation. It also checks that IRQ state stays disabled in callbacks;
three kernel workers still perform the12000-operation reader/writer stress.
Topology is fixed during this diagnostic. Two topology checks bring the
expected native total to650 per run. Target-header compilation passes; actual
native retest is PENDING USB recovery. Earlier648-check passes do not validate
this revised fixture.

Physical evidence is in ../evidence/physical-vma-001 and ../evidence/physical-preempt-001. Complete GEM/ring builds
still fail on later compatibility dependencies, preserved alongside that evidence.


Recovery update2026-11-14: the revised VMA fixture is now native validated,
four650-check runs with zero errors, two readers and actual unload. Rebuilt
completion passes four47056 checks; combined190824. The pending status above
is historical. Evidence: experimental/i915/evidence/physical-lockref-recovered-001
(relative to repository root). Whole original i915 compilation still fails.
