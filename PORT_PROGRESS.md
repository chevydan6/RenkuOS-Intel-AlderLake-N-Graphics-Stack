<!-- SPDX-License-Identifier: MIT -->
# i915 LinuxKPI experiment — 2026-11-13
Mission incomplete. Branch experiment/i915-linuxkpi starts at GPU branch commit
30dfdb32957215b4c59da3ac763ba14ab0c482d0. Existing display and patches are untouched.

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

## Next implementation task
Continue the non-RT ww_mutex/dma_resv dependency boundary from evidence/033.
Implement atomic_long operations and actual mutex state/named initialization,
then resolve stable native task identity and Linux deadlock-avoidance semantics.
Do not turn ww_mutex_lock into unconditional successful ordinary locking. Build
with real dma-fence and errno headers to expose the following dependencies.
Native kernel runtime and actual GPU execution remain unavailable/unperformed.
Request timeline/wait/retirement, GPU mappings/context/submission and completion
are still unimplemented. No HDMI source or device state was changed.
