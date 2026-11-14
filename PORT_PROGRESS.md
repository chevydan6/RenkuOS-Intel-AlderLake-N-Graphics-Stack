<!-- SPDX-License-Identifier: MIT -->
# i915 LinuxKPI experiment — 2026-11-13
Mission incomplete. Branch experiment/i915-linuxkpi starts at GPU branch commit
30dfdb32957215b4c59da3ac763ba14ab0c482d0. Existing display and patches are untouched.

Current checkpoint: both a native RenkuOS VM and the physical Intel N100 have
built, loaded, tested and unloaded the CPU kernel-services driver. Each final
suite passes 16 runs × 62 checks with zero failures.
Original Linux ring/emitter bodies compile with the guest SDK to a partial
execution object. No GPU command has run. Historical checkpoints below retain
earlier limitations; the final VM section supersedes their missing-tool/runtime
statements.

## Established baseline
Read README, architecture, build instructions, native memory progress, first execution
plan and local RenkuOS-i915-N100-investigation.md. Native work already fixed 64-bit
GGTT, preserved HDMI build 057 and demonstrated physical forcewake/reset; execution
is still unproven. Reuse these results; do not repeat the investigation's older
32-bit-GART finding as if the native branch still lacks its fix.

## Compilation checkpoint
Pinned Linux v6.6 ffc253263a1375a65fa6c9f62a893e9767fbebfa.
Original intel_ring.c Git blob 59da4b7bd262058855d18023bbec71e8e0618793.
Whole original source fails at missing gem/i915_gem_internal.h (001).
A separate native KernelExport.h attempt fails at SupportDefs.h (002).
Initial extraction rejected an extra trailing newline (003); fixed imported bytes,
not the hash check. Source Git blob verification now passes.
Unchanged function bodies intel_ring_update_space, intel_ring_reset and
intel_ring_begin compile in a freestanding x86_64 host object (004).
Private slice object layouts replace full Linux objects explicitly.
This is extraction evidence, NOT whole-file LinuxKPI compatibility.
Native object attempt still fails at missing SupportDefs.h (005).
No linked module, native executable, GPU command, MMIO or DMA test was run.
gpu_exec_probe has not been fabricated or implemented.

## Reproduce
From experimental/i915:
    make host-object
    make native-object CC=<native-or-cross-gcc> NATIVE_CPPFLAGS='<matching SDK includes>'
Generated source, ELF object and dependency file are under build/.
nm shows i915_request_timeline, panic, renku_ring_wait_for_space unresolved.
No success stub resolves GPU wait/retirement.
Local workspace /home/dan/i915-linuxkpi contains only experimental additions,
not a clone of the full GPU repository (shell DNS unavailable).
GitHub connector was used to create the real branch and preserve its base tree.

## Original checkpoint next task (superseded below)
Obtain matching RenkuOS development SDK/cross compiler or native VM access,
reproduce native-object, then implement request timeline/wait and retirement against
the retained completion path before linking ring reservation into a driver.
In parallel with that dependency closure, compile whole intel_ring.c with actual
GEM headers and inventory the next failures. Do not claim the partial layout is
binary-compatible with full Linux objects.


## Continuation checkpoint: real RenkuOS headers and complete DRM allocator

2026-11-13. Authoritative local Git checkout is now
/home/dan/haiku-intel-gpu on experiment/i915-linuxkpi.
Network clone access recovered. Source checkout /home/dan/renkuos-i915-source
is pinned to the GPU project's actual RenkuOS baseline
4799bb5a188c4bb6bbef967287640cc21a061a0d. Target source was not modified.
The earlier missing SupportDefs.h was resolved by populating the real headers;
StorageDefs.h was the next search-path failure (006), then compilation passed
(007). No replacement OS header or generated configuration shim was used.

Implemented and compiled:
- Complete unchanged Linux v6.6 drm_mm.c and drm_mm.h, used by i915 for GPU
  address-range allocation, with unchanged Linux rbtree/list/interval-tree code.
- Minimal compiler/type/bit/list dependencies; native phys_addr_t; real atomic
  acquire/release operations for reached scalar interfaces.
- Native warning adapter calling RenkuOS dvprintf; actual callback-based DRM
  printer. No allocation, lock, wait or DMA operation returns fabricated success.
- Ring slice, complete drm_mm, rbtree and diagnostics compile with -nostdinc,
  real RenkuOS headers, no host libc headers, no red zone or vector instructions.
  They also link together using a relocatable link. This is cross-source object
  compilation with Fedora GCC, not execution in RenkuOS or a loadable module.
- Eleven imported files match their original Linux Git blob and SHA256 hashes.

Tests: real upstream CPU allocator, 20,000 deterministic transitions above 4 GiB;
6,979 successful allocations, 6,067 exhaustion results, overlap rejection,
non-power-of-two alignment, fragmentation and complete-range reclamation passed.
ASan+UBSan also passed without disabling checks (019). The first sanitizer link
failed because runtimes were absent (014); matching Fedora RPMs were downloaded
and extracted in /home/dan/i915-test-libs, with no system installation.
Allocation test uses real drm_mm/rbtree, not a mocked allocator. No GPU mapping,
page-table programming, scheduling, GPU memory store or completion was tested.

Whole intel_ring.c with actual GEM headers now reaches
DRM GEM -> kref -> refcount -> atomic -> missing asm/atomic.h (018).
phys_addr_t/resource_size_t and __iomem declaration failures from 017 are resolved
by minimal declarations; no MMIO mapping implementation is implied.

Reproduce from experimental/i915:
    make target-objects RENKU_SOURCE=/home/dan/renkuos-i915-source
    make allocator-test
On this host the latter additionally requires the SANITIZER_LDFLAGS command
recorded in evidence/019-allocator-sanitized.log (local runtime extraction).
See README.md for source preparation. Build manifests and dependency files are
in build/target; preserved evidence includes manifest and defined-symbol list.
Final verified runs: 021-target-verified.log and 022-allocator-verified.log;
UBSan halts on the first error. Header dependency inventory is preserved in
evidence/target-dependencies.json.
Final partial link still has only dvprintf, panic, i915_request_timeline and
renku_ring_wait_for_space undefined. First two are actual native kernel exports;
last two remain genuinely unimplemented request integration boundaries.

## Previous checkpoint next task (completed below)
Implement the atomic/refcount/kref dependency reached by whole intel_ring.c,
using actual RenkuOS atomics and preserving Linux refcount saturation semantics.
Recompile with real GEM headers and record the next dependency. Keep complete
DRM allocator available for eventual GGTT/PPGTT virtual-range bookkeeping, but
leave page allocation/PTE programming to a real integration step.
Native runtime/VM and N100 access remain unavailable in this workspace; no SSH
identity or VM/cross compiler is installed. Native execution and first GPU command
remain outstanding, not disproven by the host environment limitation.

## Refcount and native locking checkpoint — 2026-11-13

Imported complete, unchanged Linux v6.6 lib/refcount.c, refcount.h and kref.h.
Fourteen upstream files now verify against pinned Git blobs and SHA256 hashes.
The original saturation and final-release algorithms are retained.

Implemented reached atomic_t operations using native SupportDefs atomics
(set, get, add, compare/exchange) and matching compiler release/acquire primitives.
The atomic_set name collision is contained behind a prefixed adapter, after real
native headers are parsed. Native operations may order more strongly than Linux's
relaxed operations; they do not weaken required ordering.

The complete refcount.c also requires mutex/spinlock assisted final releases.
Implemented native semaphore-backed mutex initialization/lock/unlock/destruction,
plus native spinlock/IRQ-save adapters. Interrupted mutex waits retry; other OS
failures panic rather than proceed unlocked. Plain spin_lock disables/restores
interrupts too, a stronger exclusion than Linux; saved state is copied before
unlock. Only the reached APIs are supplied. These native lock adapters have
compiled against KernelExport.h but have NOT executed in RenkuOS.

Compilation evidence:
- 023 reproduces the previous architecture atomic failure.
- 024 records upstream's signed/unsigned saturation comparison diagnostic; 026
  builds unchanged code with -Wno-sign-compare.
- 027 demonstrates signed-overflow UBSan failure at refcount.h:200 without the
  required kernel arithmetic configuration. Linux v6.6 Makefile:998 specifies
  -fno-strict-overflow; both target build and refcount test now use this exact
  flag. No sanitizer was disabled and no original expression/test was removed.
- 029 compiles the complete original refcount.c and native lock adapters, and
  resolves their internal dependencies in the partial link. Remaining new symbols
  are real kernel sem/spinlock/interrupt exports, not invented success backends.
- 030 advances whole-ring compilation into dma_resv -> ww_mutex -> rtmutex.
- Non-PREEMPT_RT ww_mutex uses struct mutex, so 033 excludes the unused RT API
  explicitly (PREEMPT_RT builds fail). It then shows the actual next requirements:
  atomic_long_t, lock-class metadata, __mutex_init, current task identity,
  atomic_long_inc_return_relaxed, mutex_is_locked; dma-fence also reaches errno.
  No ww_mutex operation has been replaced with an ordinary mutex success path.

Actual tests:
- 028: 400,000 concurrent get/put pairs, exactly one final release, rejection of
  zero-count acquisition, conditional decrement helpers, both lock-assisted
  final releases, 100 release/acquire publication rounds, and all five saturation
  categories pass under ASan+UBSan with halt_on_error. Five warning lines are
  expected outputs from intentional saturation cases.
- Host lock tests use genuine pthread mutexes, not native kernel locks. Host IRQ
  methods abort if called; no synthetic IRQ state is reported as success.
- 031: an object compiled with RENKU_NATIVE and real target headers executes its
  actual SupportDefs atomic adapter branch on the host, testing signed wrap and
  successful/failed compare exchange. It imports no kernel symbols. This is a CPU
  compatibility check, not native OS/interrupt/GPU execution.
- 032: prior 20,000-transition original allocator sanitizer regression still passes.

Commands from experimental/i915:
    make target-atomic-test RENKU_SOURCE=/home/dan/renkuos-i915-source
    make refcount-test SANITIZER_LDFLAGS='<local runtime flags from evidence/028>'
    python3 whole-ring-attempt.py --linux-source /tmp/i915-linux-source --log <new-log>
The last command returns the compiler's nonzero status. It verifies the Linux
revision, removes stale output and refuses to overwrite previous evidence.
Updated object hashes/commands/dependencies: evidence/refcount-target-manifest.json
and refcount-target-dependencies.json. Objects remain in build/target.

## Previous checkpoint next task (progress below)
Continue the non-RT ww_mutex/dma_resv dependency boundary from evidence/033.
Implement atomic_long operations and actual mutex state/named initialization,
then resolve stable native task identity and Linux deadlock-avoidance semantics.
Do not turn ww_mutex_lock into unconditional successful ordinary locking. Build
with real dma-fence and errno headers to expose the following dependencies.
Native kernel runtime and actual GPU execution remain unavailable/unperformed.
Request timeline/wait/retirement, GPU mappings/context/submission and completion
are still unimplemented. No HDMI source or device state was changed.

## Wound/wait prerequisites and fence type checkpoint — 2026-11-13

Implemented from actual failures in 033:
- atomic_long_t stamp increments use native atomic_add64, preserving 64-bit wrap.
- Named mutex initialization creates a real native named semaphore. An atomic
  held-state indicator backs mutex_is_locked; unlock clears it before publishing
  the semaphore to the next owner. It is an advisory snapshot, not an acquisition.
- Native current-task identity has scoped registration keyed by find_thread(NULL).
  The caller owns the task record and must keep its scope alive through every
  referencing Linux context. Duplicate registrations are rejected; access outside
  a registered scope panics/traps. There is no fake singleton, fake scheduler or
  claimed task-reference/grace-period support.
- A monotonic nanosecond adapter uses real system_time_nsecs in native builds and
  real CLOCK_MONOTONIC in host tests. Native runtime is still untested.
- The original Linux ww_mutex.h now compiles with -x c -fkeep-inline-functions to
  a genuine ELF ww_mutex.o, preserving original inline context and slowpath code.
  This is not a precompiled header. The partial link explicitly leaves
  ww_mutex_lock and ww_mutex_lock_interruptible unresolved. No deadlock avoidance
  or wounding operation has been replaced by ordinary successful locking.

Additional compilation-driven interfaces:
- Original Linux error-pointer and errno headers imported; all 4095 encoded error
  pointers tested in the target-header CPU probe. Native B_* statuses that expand
  through errno names are captured before installing Linux's internal errno domain;
  all six retain their native values. No ioctl status conversion is claimed.
- asm/current uses the real scope getter. wait.h's reached smp_mb is a real
  sequentially-consistent fence. bitops exposes only existing actual scalar helpers.
- DMA-fence timestamp/metadata declarations now parse. RCU callback representation
  and pointer-load operations exist; RCU readers, grace periods and reclamation
  do NOT. The unused printk/seqlock includes provide no runtime interfaces.
- linux/mm.h provides checked native 4-KiB page constants and an unresolved
  unmap_mapping_range declaration, not a recreated Linux VM or successful unmap.

Real tests and artifacts:
- 047: all target objects plus original ww inline ELF compile/partial-link; actual
  target-header atomic/errno probe executes on host (native atomic branch).
- 048: 40000 original ww_acquire_init/done/fini contexts on eight real host threads
  have unique ordered stamps and stable registered identities; mutex initialization,
  held-state and unregister/re-register checks pass under ASan+UBSan. Monotonic
  clock checks also pass. This tests prerequisites, not ww_mutex_lock/deadlocks.
- 049: original refcount's 400000 concurrent get/put pairs, final-release ordering,
  five saturation classes and lock-assisted host releases still pass ASan+UBSan.
- 050: original drm_mm's 20000 allocation/free transitions still pass ASan+UBSan.
- 23 imports (including upstream COPYING) are byte-exact with pinned Linux v6.6 Git blob/SHA256 verification.
  Latest hashes, commands, dependency closure and symbol lists are preserved in
  ww-prerequisites-target-manifest.json, ww-prerequisites-dependencies.json,
  ww-header-symbols.txt and ww-target-undefined.txt under evidence/.

Whole-file attempts 034–046 are retained. Current failure 046 is now in
DRM VMA manager: rwlock_t/read_lock/read_unlock/rwlock_init, memset declaration,
__u64, and VM area prototypes. Full Linux MM is not needed merely for PAGE_SHIFT;
no native page/DMA allocator has been simulated to move past this boundary.

FreeBSD comparison inspected ww_mutex.h blob
9219755bb78e3edbe593e34ecb6bba05c919053b and linux_lock.c blob
3cebfc6ae3bbf8db622bc35ac0c4bcf96d481e90. Its wrapper uses native sx/CV services
and a global waiter cycle check, illustrating that context/header compatibility
alone does not provide the lock algorithm. No FreeBSD code was copied here.
The attempted linux_ww_mutex.c path was absent; implementation is linux_lock.c.

Reproduce from experimental/i915:
    make target-atomic-test RENKU_SOURCE=/home/dan/renkuos-i915-source
    make ww-context-test SANITIZER_LDFLAGS='<local runtime flags in evidence/048>'
    make refcount-test SANITIZER_LDFLAGS='<same local runtime flags>'
    make allocator-test SANITIZER_LDFLAGS='<same local runtime flags>'
    python3 whole-ring-attempt.py --linux-source /tmp/i915-linux-source --log <new-log>

## Full-compatibility next task (still pending)
Implement actual ww_mutex lock/unlock/trylock with native contention/wakeup and
correct -EDEADLK/-EALREADY semantics, retaining source algorithms where practical.
The registered task scope supplies identity, not task lifetime/scheduling; any
waiter must retain valid storage throughout blocking and cancellation.
In parallel, narrow the newly reached DRM VMA rwlock/VM interface from 046 without
building the entire Linux VM. Real DMA-fence waiting/signaling and RCU lifetime
remain unresolved. Compile headers without confusing them with executable services.
No native kernel run, driver install, GT/MMIO operation, GPU mapping, submission,
completion or hardware result occurred. HDMI sources remain unchanged.

## Original Linux batch-start emitter checkpoint — 2026-11-13

Added actual hardware command-construction code alongside the full-file
LinuxKPI experiment: unchanged gen8_emit_bb_start_noarb and gen8_emit_bb_start
from Linux v6.6 gen8_engine_cs.c now compile/link with the existing ring slice.
The extraction retains the complete bodies and original intel_ring_advance
checks, and imports the full original intel_gpu_commands.h definitions.
Linux's execlists source selects these emitters for the pre-12.50 graphics path;
no engine has actually been initialized or selected on hardware here.

Compiler evidence 054 exposed conflicting u64/IS_ENABLED/power-of-two definitions
between the initial isolated ring shim and the accumulated compatibility layer.
The ring now consumes the common native-width types and original Linux error
helpers; command-header sign-extension/mask helpers are real arithmetic.
No hardware opcode/packet algorithm was rewritten. The private request adds a
native software nopreempt attribute as the query boundary. These private layouts
are still NOT complete Linux request structs; do not mix them with full i915
request objects. Secure dispatch is an internal trusted flag, not a userspace ABI.

Artifacts/tests:
- 055: both original emitter functions compile as an object referencing actual
  intel_ring_begin plus panic.
- 056: 16 ASan/UBSan integration cases cover normal/noarb paths, software
  nopreempt, trusted dispatch selectors, >4-GiB batch addresses, ring wrapping,
  guard regions and no implicit publication of the software tail.
- 057: the same tests pass while linked to the actual target-header-built
  emitter/ring objects. This executes CPU command construction, not GPU work.
  Any attempted timeline/wait in the test aborts; no simulated completion exists.
- 058–061: ring compilation, 40000 ww context transitions, 400000 refcount pairs
  and 20000 allocator transitions still pass (the CPU tests retain ASan+UBSan).
- 053 additionally verifies target-header atomics, errno/error pointers after the
  native failure paths were made trap if a debugger resumes after panic.
- 25 imported files verify unchanged. Updated emitted object/dependency evidence:
  emitter-target-manifest.json, emitter-target-dependencies.json and
  emitter-target-symbols.txt under evidence/; binaries remain under build/.

The latest full intel_ring.c attempt 062 still fails: real rwlock APIs/types
in drm_vma_manager, then __user/raw_spinlock completion and IDR/radix-tree header
compatibility. The simpler memset declaration, __u64 and VM-area forward types
from 046 were supplied using native C headers/declarations. No mmap, unmap,
completion, ID allocator or rwlock operation has been faked to pass compilation.

Reproduce from experimental/i915:
    make emitter-test SANITIZER_LDFLAGS='<local runtime flags from evidence/056>'
    make target-emitter-test RENKU_SOURCE=/home/dan/renkuos-i915-source
    make host-object

## Next implementation task (pre-VM checkpoint; superseded below)
For the full LinuxKPI path, implement actual ww lock/deadlock/wakeup semantics and
native rwlock support from 062, then continue the GEM/VMA dependency closure.
For the bounded execution slice, reuse the existing native pinned-memory/context
results to design the backing/mapping integration before attaching this emitter
to a real engine. Required GT setup, context, batch/result mappings, submission
and completion remain absent. Do not treat CPU packet construction as a store
executed by the GPU or create gpu_exec_probe until those services exist.
No full-port superiority, native kernel runtime or GPU success is claimed.

## Native VM checkpoint: built, loaded, exercised and unloaded

2026-11-13. QEMU 10.2.2/KVM now runs actual RenkuOS build00081, source
af22bfb13d758e13a2d2f6ab204bdbc3e4aab42f, two CPUs and 768 MiB. The local
image matches its published SHA256. Session 004 uses a private qcow2 overlay,
standard VGA/e1000, no GPU passthrough or host system package changes.
Matching Renku haiku_devel hrev60072_780 and native GCC15.3 are installed.
This SDK is separate from the older 4799bb5a cross-source baseline.

Implemented and verified in the actual guest kernel:
- Legacy driver entry points, native module glue/import-library link,
  permission-checked ioctl client, actual load and unload.
- Wired 4-KiB areas, physical-address lookup, CPU pattern integrity, deletion
  and repeated-destruction error. CPU physical backing is NOT device DMA.
- Native semaphore mutexes, IRQ-excluding native spinlocks, task registration
  and original ww context stamps on two actual kernel workers per run.
- Original complete drm_mm/rbtree range allocation above 4 GiB and overlap
  rejection; original kref/refcount get/put with exactly one final release.
- Explicit native semaphore completion resources: signal, block, absolute
  monotonic deadline, interruption retry, negative/overflow timeout errors,
  deleted-resource errors and cleanup. Linux completion/waitqueues remain open.
- Invalid ioctl, length and user pointer returns through real user_memcpy.

Artifacts: build/native/i915_linuxkpi_test.driver, linuxkpi_native_probe and
i915_execution_preparation.o. The last contains original extracted Linux
intel_ring/gen8 functions compiled with the native guest compiler and SDK,
separate from the installed test driver. Timeline/wait imports remain undefined.

Final evidence: experimental/i915/evidence/native-vm-006/. Sixteen runs pass
62 checks each; total 64 pages allocated/freed, 32 kernel workers and 64000
iterations of each lock/reference path. Concurrent clients exercise serialized
ioctl entry; each ioctl has two concurrent workers. Requested 25000-us timeouts
measured 25060–25284 us. This is functional evidence, not a GPU benchmark.
Serial INIT/UNINIT status=0 verifies actual driver load and semaphore teardown.

Failed attempts preserved:
- 063 used nonexistent B_BAD_AREA_ID; native source establishes B_BAD_TEAM_ID
  for deleted-area lookup. Corrected native backend/driver compile is in 064.
- 065: guest lacked GCC; matching SDK/toolchain installed.
- 066: first real run panicked on a late kernel-worker join. Exact guest source
  establishes no cached death entry for kernel threads. Corrected handling
  requires both completion and absent thread ID; other errors remain fatal.
  Some unflushed build files were lost on reboot; runner now syncs before tests.
- 067: immediate reinstall reached ENOENT before asynchronous devfs publication.
  A bounded readiness wait fixes this; functional checks remain intact. Final
  suite additionally tests duplicate cleanup and invalid semaphore operations.

Reproduce using experimental/i915/vm/README.md: prepare a hashed source archive
with `python3 vm/prepare-bundle.py --output <new-file>`, transfer/extract in the
guest, and run `bash vm/run-native-tests.sh`. Local ISO, overlay, binaries and
detailed logs remain under /home/dan/i915-vm. Reviewed text evidence is committed;
private SSH keys are excluded.

## Next implementation task

Native CPU service validation is complete for this subset. Next close the real
full-file rwlock/ww/GEM failures from 062, or connect the native-compiled bounded
ring/emitter slice to owned GPU mappings and actual GT/context/submission/
retirement. Reuse existing native physical memory/context findings. Neither CPU
physical addresses nor command encoding provide valid DMA/GPU mapping.
gpu_exec_probe remains absent until real execution mechanisms exist; a CPU
write must not stand in for its store. Physical N100 submission/completion and
the full-port comparison remain open.

## Physical N100 checkpoint

The user supplied access to the live N100 through NanoKVM. Recovered its actual
authenticated MCP screenshot/keyboard interface and used the existing KVM-side
physical-test lock. Original attached image/media and Intel/display drivers
were preserved. Absolute mouse input failed with ENXIO; keyboard control works.

Fullscreen Terminal inspection confirms RenkuOS hrev60072+518, four Intel N100
cores, host bridge 8086:461c and GPU 8086:46d1. Boot volume is RenkuTest on
/dev/disk/usb/0/0/1. Existing GCC13.3 and haiku_devel hrev60072_518 match this
older physical kernel; no VM binaries or mismatched SDK were installed.

Transferred the exact verified source bundle from the VM experiment. Actual
native compilation and tests were launched using NanoKVM keyboard into the
visible Terminal; SSH transferred files and collected logs. The native driver,
client and original Linux execution partial object all compile on the N100.
The physical suite passes 16 × 62 checks, 64 page allocations/frees, 32 real
kernel workers and 64000 lock/reference-path iterations. Requested 25000-us
timeouts measured 25003–25023 us. These times are not a GPU performance result.

Evidence is experimental/i915/evidence/physical-n100-001/, including native
compiler commands, all client results, PCI/CPU identity excerpts, artifact
hashes and real INIT/UNINIT status=0. Strict postchecks confirm the experimental
test driver and device were removed. Raw archives/binaries and the actual
NanoKVM screenshot remain outside Git under /home/dan/i915-vm; a local artifact
copy also exists at experimental/i915/build/physical-n100-001/.

This resolves physical validation of the selected CPU kernel-services subset.
No GPU commands, engine initialization, GPU mappings or Intel-driver replacement
occurred. The execution object's timeline/space-wait imports remain unresolved;
the next implementation task above is unchanged. gpu_exec_probe is not present.

## Owned GGTT mapping candidate — continuation 2026-11-13

Implementation is under experimental/i915/mapping/, outside the native global
patch series. It reconstructs the pinned native baseline plus 0001..0096, adds
N100 UC GGTT invalidation to the GART callback, and exposes a private memory-only
owner interface. One new Request lease owns four non-reserved allocations,
private CPU aliases, checked GGTT offsets and explicit cleanup. No second GART
owner, display-lease borrowing, direct experimental PTE writer or GPU publication
is introduced. Existing Linux imports remain byte-identical (25 verified files).

The source and ABI deliberately expose neither CPU physical nor per-device DMA
addresses. This does not establish DMA admission, engine cache/TLB correctness,
context setup, submission or retirement. The ring/timeline imports remain open.
The four buffer names are backing roles, not valid hardware structures.

Actual owner bodies pass UBSan host checks for bounds/overlap, reentry, every
allocation/free failure, failed-output preservation, display closure, kernel
alias permissions, alias deletion failure and cleanup retries. Three generated
address-domain/double-free regressions are rejected. Evidence: mapping-host-001.

Two candidates compiled visibly on the real N100 with its matching SDK/GCC13.3.
The first physical test panicked on the first CPU store: SMAP rejected access to
the existing user-permission aperture. Trace screenshot/raw source archive are
preserved under /home/dan/i915-vm. The corrected owner clones the same aperture
cache into a kernel-only alias (native VM preserves WC/UC attributes), keeps
original GART addresses separately, and deletes the alias after all frees.

Before the corrected hardware test, NanoKVM HID writes and target filesystem
reads stalled before its run script started. Recovery used SSH shutdown requests,
a gadget rebind and the official HID-reset endpoint. The latter unexpectedly
selected the default NanoKVM media; the original /root/renku-gpu-test-004/disk.img
and ro=0/cdrom=0 attachment were restored and checked immediately. Do not use
HID reset without preserving/restoring the custom media. Dan manually restarted
the N100; the original external RenkuTest boot path and
candidate hashes were verified again before testing. No results are attributed
to that interrupted attempt.

The corrected mapping probe then passed eight runs of 38,927 checks each: 311,416
checks, zero failures. Each set owns 19 pages in four GART objects, with offsets
0x3c00000/0x3c01000/0x3c02000/0x3c03000 and sizes 4K/4K/4K/64K. All words
pass two CPU-pattern write/read rounds through the kernel aperture alias. This
establishes owned native GGTT backing and CPU aperture access/release, not a GPU
command, per-device DMA mapping or an engine translation/cache contract.

The first wrapper returned 1 after four successful runs because asynchronous
devfs removal had not finished and its unload grep matched historical messages.
That failure remains recorded. The corrected runner's next four runs and bounded
cleanup return 0, with new INIT/UNINIT status 0 and absent diagnostic files/device.
Native GART logs record binding/unbinding. Final strict postchecks pass and HDMI
remains live. Evidence: experimental/i915/evidence/physical-mapping-001/.

Final owner/GART candidates remain installed on the external test volume. The
native display driver, accelerant and kernel were not replaced. Original owner
and Intel GART binaries remain under /boot/home/i915-mapping-001/recovery/.
A checked rollback-to-original.sh is in /boot/home/i915-mapping-002/; it restores
those originals on disk and requires reboot. It was not executed. Future work
must retain the memory-only API prohibition on device publication until DMA,
GT/context, cache visibility and completion/reset lifetime contracts exist.

Session cleanup completed: the dedicated transfer SSH listener was stopped,
the physical lease released, and the original custom NanoKVM image attachment
verified. The actual HDMI Terminal remains running with successful results.

## Linux DMA API port checkpoint — 2026-11-13

Continue the original i915 port through OS compatibility rather than adding
native GPU algorithms. Five additional byte-exact Linux files bring the verified
import set to 30, including original i915 GEM GTT/physical objects and SG code.
Their full translation units still fail; actual compiler diagnostics are retained.

The new Linux DMA API adapter delegates to existing unchanged RenkuOS bus_dma
and malloc implementations. Native callback addresses, coherent order alignment,
mask/lifetime checks and partial chained-SG rollback are implemented. It supports
the scoped x86 direct/coherent path; no PCI attachment, IOMMU programming or GPU
execution is implied. The page view is limited to caller-owned wired backing.

A reproducible bundle builds in actual N100 fullscreen Terminal with GCC13.3 and
SDK518. Four runs ×45,116 checks =180,464 pass with zero failures, real teardown
and unchanged graphics publication. Host ASan/UBSan API tests and two mutation
controls pass. A lazy coherent-alias mapping panic was corrected and preserved.
See experimental/i915/dma/README.md and evidence/physical-linux-dma-001/.

The next original-i915 boundary is complete GEM/ring dependency compilation:
compiler annotations/bit helpers, rwlocks/completion, DRM/GEM/VM objects and real
PCI lifecycle. Hardware algorithms should remain in upstream i915. Neither these
DMA CPU tests nor earlier native GGTT tests establish a GPU store or Mesa support.

## Complete DRM VMA dependency port — 2026-11-13

Original Linux compiler_types/log2 headers and the complete DRM VMA offset manager
are now imported unchanged (34 verified files). Compiler annotation/stdarg/log2
conflicts and gfp_t propagation are corrected in OS wrappers. The VMA module
compiles as a whole against real RenkuOS headers and with the actual native SDK.

Native adapters use existing rw_spinlock services with wired per-CPU IRQ/nesting
state, and sleeping native heap allocation plus native deferred_free for frees
inside a spinlock. General Linux allocation/VM services are not fabricated.

The N100 passes four ×646 checks and 48000 total lock operations: concurrent
readers, exclusive writer, IRQ restoration/nesting, offset/access semantics and
actual teardown. Host ASan/UBSan OOM/access tests and a revocation mutation control
pass, as do existing component sanitizer and target-header probes. Evidence is
experimental/i915/evidence/physical-vma-001/ and vma-host-001/. No GPU work ran.

Whole GEM/ring builds advance past those earlier errors but still fail at later
Linux type/architecture helpers, raw spin/wait/lockdep integration and missing
external sparse-checkout UAPI headers. Their failed compiler logs are preserved.
Continue porting those reached services; do not replace original GPU algorithms.

## Original completion and simple-wait-queue port — 2026-11-13

The complete original Linux completion.c and swait.c algorithms and public
headers compile and run on RenkuOS through native semaphore/deadline/IRQ/task
scope adapters. Original do_div, kconfig, lockdep-off and x86-64 UAPI helpers
resolve earlier scalar/arithmetic/raw-spin/class-key issues. The manifest now
verifies 48 unique files and rejects duplicate paths. No GPU algorithm changed.

Actual N100 GCC13.3/SDK518 compilation is warning-free. Four 119-check runs pass
with zero failures and 96 kernel waiter workers. Counted wakeups, two queued
waiters, broadcasts/reinitialization, timeouts, IRQ-disabled completion and
scoped-event cleanup pass. Real user-thread SIGUSR1 interrupts an interruptible
wait with -ERESTARTSYS; a killable wait ignores that ordinary signal and finishes
on completion. Killing a waiter is not tested. Actual diagnostic unload 0 and
unchanged graphics publication pass. Evidence: physical-completion-001.

Host ASan/UBSan completion tests and a lost-count mutation control pass; existing
allocator/refcount/ww/emitter, DMA, VMA and target-header probes also pass. Native
whole GEM/ring attempts are now performed on RenkuOS as well as the development
host. All still fail at later header namespace and architecture/preemption
interfaces; their real exits and errors are retained. No full i915 build or GPU
execution is claimed. Continue integrating original code with real OS services.


## Native preemption and canonical helper integration — 2026-11-13

The port now uses 65 unique byte-exact Linux v6.6 imports. Original compiler
attributes, build_bug/minmax, arithmetic/alignment and byteorder headers replace
conflicting local macros. Native libc is parsed before Linux attribute and swab
spellings; native __packed and wait-status collisions are resolved in wrappers.
Early might_sleep/init/type visibility is corrected without editing GPU code.

New preemption exclusion delegates to real native IRQ masking. One wired per-CPU
scope manager now serves spinlocks, rwlocks and preemption together, preserving
requested IRQ state across nested or overlapping lifetimes. This is stronger
than Linux task-only exclusion and requires short nonblocking sections. Actual
IRQ-context classification and Linux hardirq/softirq/NMI counters remain absent.

On the physical N100, GCC13.3/SDK518 builds and four runs each pass: 149 completion/
preemption checks, 648 VMA checks, 62 older KPI checks (596/2592/248 total). VMA
includes 48,000 stress operations and required reader overlap. Diagnostics unload
with status0 and graphics publication unchanged. The initial VMA setup missed
reader overlap and failed; a bounded two-reader rendezvous strengthens the setup
without removing any assertion. Failed results and exact source hashes remain.
Host ASan/UBSan models and compiler/mutation controls also pass. Evidence is in
experimental/i915/evidence/physical-preempt-001 and preempt-host-001.

The temporary loss of USB/HDMI/SSH occurred before any new module installed;
the user restored the connection. The interrupted archive failed its checksum
and was preserved, then retransmitted and verified. No cause is inferred.
Final native checksum verification passes all 6970 non-Markdown bundle files,
including the revised test. HDMI/media baseline was preserved; SSH stopped and
the physical lease released after evidence collection.

Complete original GEM/ring compilation still fails on RenkuOS: GTT reaches
asm/ioctl.h, physical GEM reaches device-number macros/RCU and asm/cache.h, ring
reaches bitmaps/bit scans/loff_t/cpumask. Actual failed compiler exits are retained.
These define the next integration steps. Full i915 build and GPU execution have
not yet been demonstrated; original GPU algorithms remain unchanged.

## Original bit operations and bitmap-search port — 2026-11-13

87 unique byte-exact Linux imports now include complete find_bit.c/hweight.c,
bitops/bitmap/find headers and generic bit-scan/non-atomic/weight algorithms.
Native/compiler atomic mutations and original x86-64 ioctl encoding complete
this step. Header-order and long-width checks cover22 host/target combinations.

Actual RenkuOS GCC13.3/SDK518 compilation and four final46,389-check runs pass
(185,556 total), retaining completion/preemption cases and96 waiter workers.
Four648-check VMA and four62-check KPI regressions also pass with actual unload,
48,000 VMA stress operations and unchanged graphics publication. Host ASan/UBSan
passes46,362 checks; independent search oracle and first-offset mutation catch
regressions. Invalid GENMASK, mixed-min and build_bug compile controls pass.

Original find_bit.c emits maybe-uninitialized warnings on direct private zero-size
nth entry. Only that warning is downgraded from Werror for that source; warnings
are preserved. Public nth wrappers guard n>=size; tests use these APIs, including
NULL/zero-size checks. A scoped original-header type-limits warning exception
retains actual GENMASK invalid-input errors. See completion/README.md for limits.

Whole native ring compilation now passes the earlier bitmap helpers/loff_t/layout
errors and reaches cpumask configuration plus asm/cache.h. GTT passes asm/ioctl.h
and reaches random/static-key/atomic helpers, U32_MAX, cpumask and asm/param.h.
Physical GEM still reaches device-number/RCU/cache dependencies. All three fail;
exact native errors are preserved in physical-bitmap-001. No full driver or GPU
execution is demonstrated. General lib/bitmap.c allocation/parsing is not supplied.

## Original CPU masks and architecture integration — 2026-11-13

99 unique unchanged Linux imports now include complete cpumask, cache/linkage,
limits and parameter headers. SMP/64-CPU capacity replaces the accidental UP
fallback. A native fixed-topology snapshot obtains real CPU count and enabled
flags through existing kernel services; Linux mask layout stays separate from
native CPUSet. Disabled IDs remain present but are excluded from online/active.
CPU hotplug, affinity and NUMA scheduling are still outside this adapter.

Actual N100 GCC13.3/SDK518 builds and four 46555-check runs pass (186220 checks),
reporting 4 CPUs/4 online/mask 0xf. CPUID matches the selected 64-byte cache alignment.
Four648-check VMA and four 62-check KPI regressions pass with actual unload,
unchanged graphics publication and 48000 VMA stress operations. All 7046 payload
files match before and after native testing. Host ASan/UBSan, disabled-CPU mutation,
old regression/mutation controls and 44 first/repeated-header checks pass.

Whole native builds advance: ring reaches generated/bounds.h through VM page
layout; GTT reaches asm/spinlock_types.h; physical GEM retains device-number,
RCU, __clear_bit_unlock and bounds dependencies. Actual failures are preserved
in physical-cpumask-001. No fake page structures or scheduler were supplied.
No complete i915 build or GPU execution is demonstrated. See completion/README.md
for fixed-topology ownership, namespace choices and original search warnings.

## Original raw lock layouts and bit-spin port — 2026-11-14

104 unique unchanged Linux imports now include original raw/regular/RW lock
structures and complete bit_spinlock.h. Architecture storage delegates to native
spin/rw locks; regular and raw types are distinct and original initializers work.
Shared per-CPU IRQ scopes retain restore state. Native bit-lock algorithms use
actual preemption exclusion and release/acquire bit operations.

Actual N100 GCC13.3/SDK518 compilation and four 46786-check runs pass (187144),
including eight bit workers and 32000 protected updates. Four648-check VMA and
four 62-check KPI regressions pass with actual unload, unchanged graphics and
48000 VMA stress operations. All 7058 payload entries verify before and after tests.
Host sanitizer 46742 checks, models, 54 header-order controls, 19 native-source probes
and old regressions pass. Link diagnostics exposed a previously optimized-away
__bitmap_equal dependency; its complete original function is now extracted from
manifested lib/bitmap.c. Dirty-tail mutation is rejected; no full bitmap port claim.

NanoKVM input failed before the new code transferred. User restarted RenkuOS;
keyboard control recovered and all native work then ran visibly in Terminal.
Cause of the earlier input failure is not inferred. Media/display baseline is
preserved; temporary SSH stopped and lease released after collection.

Whole native original files still fail: ring needs VM bounds/page layout; GTT
also reaches logging, task-name and va_format declarations; physical GEM retains
device-number/RCU/bounds dependencies. Exact errors are in physical-raw-lock-001.
No full i915 build/GPU execution is claimed. Continue original service/layout
adaptation without substituting fabricated Linux page objects.


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
for this checkpoint. See experimental/i915/evidence/physical-lockref-001 and lockref-host-001.

Cross-source whole-file attempts advance past the prior bounds includes:
ring/GTT now fail in Linux sched/mm.h on missing real process-mm/task integration;
physical GEM fails on seqcount/seqlock types and asm/stat.h. These are cross
compiler results, not new native whole-file attempts. Full i915 compilation
and GPU execution remain unproven.


## 2026-11-14: lockref/VMA recovery validation

The pending VMA fixture now passes actual RenkuOS compilation and four650-check
runs, with two readers, zero worker errors,48000 stress operations and unload0.
The same native lockref/completion source rebuilt and passes four47056 checks;
all eight runs total190824 checks, zero failures. Native source checks pass
before/after. Whole native ring/GTT now reach sched/mm.h process-mm/task services;
physical GEM reaches seqcount/seqlock and asm/stat.h. Full i915 build/GPU execution
remain absent. See experimental/i915/evidence/physical-lockref-recovered-001.


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


## 2026-11-14: managed folio checkpoint (native validation pending)

Managed wired allocations now expose canonical folio heads, actual shape/order
queries and original refcount ownership. Original folio helpers, const selection,
local mapping bodies and the complete pagevec header are retained. Batch release
consumes real references and queues final deletion through the existing joined
worker. Order-zero folios remain noncompound. Unsupported exceptional swap/DAX
batch handling stays unresolved; no raw page flags or LRU/cache states are faked.

Host ASan/UBSan passes47416 checks, including local/atomic concurrent users and
batch reference balance. Tail-redirection and dropped-batch-release mutations
are rejected. All94 include combinations,18 configuration/type rejections,
four unresolved link controls and host VMA/DMA regressions pass. Native test C
units compile against the actual pinned headers. All118 imports match their
pinned source paths, Git blobs and SHA256 hashes. See
experimental/i915/evidence/folio-host-001 and completion/README.md for scope.

These additions have NOT run on RenkuOS. USB HID writes still time out before
transfer/compilation; custom SSH is stopped and known-key default SSH recovery
times out. A physical restart was requested. The previous physical-pages-001
version7 result remains the last native checkpoint. Version8 native tests are
prepared with seven allocations/frees per run; no new native count is claimed.

The complete original GEM shmem source is now imported and included in the
whole-source native compiler loop. Cross compilation still fails in actual
shmem/swap/mmzone dependencies at rejected VM bounds and missing asm/page.h.
Its cache, pin/unpin, dirty/accessed, truncate/writeback and file/mapping lifetimes
must connect to real native VM services. Ring/GTT still need process-mm/task
integration. Full i915 compilation and GPU execution remain unfinished.


## 2026-11-14: persistent pageable-store backing (native pending)

The internal backing layer now separates store lifetime from folio pin lifetime.
Its native bridge creates B_NO_LOCK anonymous areas, wires individual writable
pages with lock_memory_etc, queries real physical extents while pinned and queues
matching unpins on final folio release. Re-pin queries physical identity again;
contents remain owned by the store. Concurrent first pins merge into one head,
with redundant native pins released. Live, pending and in-flight pins block store
destruction; empty stores still block backend shutdown. Failed area deletion
keeps the owner available for retry. New code is MIT; all118 Linux originals
remain unchanged. This is an internal provider, not yet Linux shmem/file glue.

Final host003 and its restored binary pass49504 ASan/UBSan checks, including data
retention, failure recovery, blocked pin/unpin deletion and two concurrent workers.
Four new mutations reject missing unpins, duplicate heads and live deletion.
Header controls94/18/four and actual target-header C core/test/client compiles
pass. Report version9 has112-byte layout and one store create/free plus two
workers/256 iterations. Sources, exact logs and native API source-contract hashes
are in experimental/i915/evidence/store-host-001.

These are host results. The C++ cross probe lacks the matching target <new>
header, so the new native bridge has no C++ build/link/runtime validation yet.
USB input still fails; the read-only screenshot shows the previous version7
native page checkpoint. No new native code/module was transferred or installed.
Both version8 folio and version9 store runtime tests await hardware recovery.
Native VM reclaim/swap behavior remains untested. No lease was acquired for the
host work, and the previous native lease remains released.

Next connect native-backed store ownership to actual Linux file/address_space
and shmem APIs, including real GFP, pinning, dirty/accessed, writeback, truncate
and mapping lifetimes. Whole original i915 files still fail in VM/process
services; the full driver and GPU execution remain unfinished.


## 2026-11-14: physical folio/store validation

The unchanged version 9 candidate now compiles and links with native GCC/G++13.3
on the physical N100 hrev60072+518. Four49556-check completion/memory runs and
four650-check VMA regressions total200824 checks, zero functional failures.
All7111 source checks pass before/after; both diagnostics unload successfully,
13 production component hashes match and app_server remains PID127. See
experimental/i915/evidence/physical-store-001 for native logs, artifact hashes,
clock caveat, exact environment and retained whole-source failures.

The owner clarified that SSH may run development/builds/tests, with progress
visible through NanoKVM. The prior SSH restriction is superseded. Native access
and pending C++/folio/store validation are resolved for this checkpoint.

The full i915 driver still does not compile. GTT/ring require real process-mm/task
integration; physical/shmem GEM require native shmem/VM interfaces beyond the
internal store. No reclaim/swap-pressure, GPU DMA, submission or retirement is
established. Continue genuine file/address_space/shmem integration and original
whole-source closure; do not manufacture VM bounds or successful missing APIs.



## 2026-11-14: native shmem file ownership

Candidate015f1632 connects real pageable backing to scoped Linux file/inode/
address_space objects and shmem setup/read APIs. Final fput retains area and
metadata through outstanding folios and in-flight pins; joined-worker deletion
and callback retirement complete final cleanup. Native no-reserve flags are
forwarded and observed. Unsupported GFP/atomic policies fail explicitly.

Focused ASan/UBSan tests reject six lifetime/policy mutations; the combined host
suite passes57729 checks,100 header cases pass and all118 imports stay unchanged.
Independent read-only review found no material defects. Native GCC/G++13.3 then
passed four57786-check runs plus four650-check VMA runs:233744 total checks, zero
failures, actual unload, matching13 production hashes and preserved app_server127.
See experimental/i915/evidence/physical-shmem-001 for hashes, source revision,
clock caveat, native logs and full-source compiler failures.

This is scoped ownership, not complete shmem/GEM. Current i915 HIGHUSER/reclaim/
retry masks still need real native allocation-policy support. Dirty/accessed,
writeback/truncate, unevictability and user mappings remain unresolved. Physical/
shmem GEM still stop at Linux VM bounds and asm/page.h via their swap.h include;
GTT/ring still require process-mm/task services. Full driver compilation, GPU DMA
and command submission/retirement remain unfinished.


## 2026-11-14: native pinned-page state bridge

Candidate a3d17e11 (tree e0990e57) adds a sleepable internal C ABI for real native
modified/accessed flags under the page cache lock, with previous-state reporting.
A retained writable pin protects physical identity. This is a prerequisite for
Linux atomic setters; those setters and dirty/writeback semantics remain absent.

Native version11 passes four57807-check completion/memory runs and four650-check
VMA runs:233828 checks, zero functional failures, actual unload,7118 source hash
checks before/after,13 unchanged production hashes and app_server127. Independent
native-field tests reject omitted-dirty and omitted-accessed writes with7 and4
failures respectively and clean unloads; restored source passes. Host57729 plus
controls,100 header cases and target-header C compilation pass;118 imports remain
byte-exact. Independent review found no material issues; both minor suggestions
were incorporated. See experimental/i915/evidence/physical-page-state-001.

Next implement managed Linux page dirty/accessed ownership, ordering, newly-dirty
return semantics and native propagation before unpin. Arbitrary GFP/reclaim,
writeback/truncate, unevictability, user mappings and process-mm/task integration
remain. All four full-original compiler attempts still fail at genuine Linux VM
or process dependencies. Full driver compilation and GPU execution are unfinished.
