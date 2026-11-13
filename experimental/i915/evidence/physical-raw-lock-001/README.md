<!-- SPDX-License-Identifier: MIT -->
# Original raw lock structures and bit-spin algorithms on native N100

Actual RenkuOS hrev60072+518, installed GCC13.3/SDK518. Compilation, execution
and whole GEM/ring attempts ran visibly in fullscreen Terminal via NanoKVM.
SSH transferred files and collected evidence only. All 104 original Linux imports
match the pinned v6.6 Git tree; no original source algorithm was edited.

Four runs per suite pass with zero failures and actual diagnostic unload status 0:

| Suite | Checks/run | Total |
| --- | ---: | ---: |
| Raw/bit locks + prior bitmap/CPU/completion/preemption | 46786 | 187144 |
| Complete original DRM VMA regression | 648 | 2592 |
| Older native kernel-service regression | 62 | 248 |

Original raw_spinlock_t, spinlock_t and rwlock_t headers and static initializers
wrap real native architecture storage. Regular methods select rlock; backend raw
and RW methods use raw_lock. Raw and regular types are distinct, with no cast
between independently invented objects. Native IRQ restore state remains in the
shared per-CPU scope manager. RT/debug-spin/lockdep instrumentation is unsupported.

Complete original bit_spinlock.h uses actual preemption/IRQ exclusion plus
acquire/release bit operations. Cases check failed trylock preserving outer state,
both unlock forms preserving flags, nesting and native static initializers. The
non-atomic release-store form requires all flags in the word to share that lock.
Eight total bit workers perform 32000 protected updates, with counter/mirror and
final flag checks. Workers queue at a completion gate, so their lifetimes overlap;
there is no contention counter or exhaustive scheduling claim. Ninety-six prior
waiter workers remain, plus real SIGUSR1 -512 and ordinary-signal killable 1949
controls. Actual kill is not tested. VMA retains 48000 lock stress operations and
reader overlap. No GPU register, GPU IRQ or submitted command is tested.

A host linker failure exposed __bitmap_equal, previously optimized away in
constant-size calls. Its containing lib/bitmap.c is imported byte-exact. Only the
complete unchanged __bitmap_equal function is extracted and linked, with original
function/source hashes in completion/bitmap_equal_slice.json. This is not a full
bitmap library port. Independent comparisons check empty, partial, multiword,
dirty-tail and differing-bit cases. Ignoring the tail mask fails its host mutation
control. This helper no longer depends on constant inlining to link successfully.

The physical source is native-bundle-001, directory /boot/home/i915-raw-lock-001,
without overlay. All 7058 entries verify before and after tests. Full manifests,
source archives, actual binaries and raw checks/results remain outside Git at
paths and hashes in result.json/source-checks.json. Separate completion-build and
vma-build snapshots avoid build/ reuse ambiguity; KPI artifacts are build/native.
Original find_bit.c maybe-uninitialized warnings and public nth-entry limits
remain unchanged; this is not a warning-free build.

The prior input failure preceded any new transfer/install. User restarted RenkuOS,
then the live desktop, keyboard, source checksum and actual native builds were
verified. No cause is inferred, and no NanoKVM HID/media reset was used. Final
screenshot shows successful suites, source check 0, failed whole-source attempts
and SSH stop 0. Dedicated SSH2222 then refused connections, lease released, custom
disk.img ro 0/cdrom 0 and configured USB preserved. Diagnostic drivers/links are
removed and graphics publication is unchanged; original display/owner/GART remain.

All three whole original translation units still fail (exit 1 each). Raw lock type
and __clear_bit_unlock issues are resolved. Ring reaches generated/bounds.h via
VM page layout. GTT reaches logging/task-name/va_format declarations and bounds;
physical GEM retains device-number/RCU/bounds errors. Actual native compiler
logs are in whole-build/. No dummy page layout/bounds or RCU success was supplied.
A full i915 build or GPU execution has not been demonstrated.
