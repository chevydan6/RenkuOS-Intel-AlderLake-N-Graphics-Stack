# GPU next: memory ownership checkpoint

The verified HDMI desktop baseline remains `gpu-hdmi-cleanup-metal-057` (`eeb334df`). Offline work uses `/home/dan/renkuos-development/source/gpu-next-worktree`, branch `gpu/n100-memory-next`. No laptop, VM, NanoKVM, boot/media, host configuration or shared build resources were accessed.

## Patch 0065

Source commit `7cba9585`: `intel_extreme: roll back undelivered graphics allocations`.
Export: `gpu-driver/patches/0065-intel_extreme-roll-back-undelivered-graphics-allocations.patch`.

Production `device.cpp` now rejects undersized allocation/free requests before copying user memory. If allocation succeeds but delivering its descriptor fails, normal `intel_free_memory()`/GART teardown releases it. Failed teardown propagates its error and preserves GART's tracked backing; the driver never frees pages independently of the mapping lifecycle. If teardown fails, the caller has no delivered retry handle: backing remains tracked by GART but this patch does not solve orphan recovery or complete per-client ownership. No new ioctl, ABI version, mapping or GPU command was introduced.

Validation:

- 116 production-case host checks pass with AddressSanitizer and UndefinedBehaviorSanitizer: every short length, input faults, bad magic, allocation failure, successful delivery/free, output failure, teardown failure and 64 consecutive failed deliveries.
- The same tests against `eeb334df` fail 106 assertions, including leaked allocation after failed delivery.
- Five existing shared-layout ioctl cases pass.
- Source diff whitespace check passes.

Commands:

```sh
python3 gpu-driver/workstreams/gpu-next/memory-ioctl-host.py source/gpu-next-worktree/src/add-ons/kernel/drivers/graphics/intel_extreme/device.cpp
python3 gpu-driver/tests/shared-layout-ioctl-host.py source/gpu-next-worktree/src/add-ons/kernel/drivers/graphics/intel_extreme/device.cpp
```

The test generator supports `--emit-cpp PATH` to emit the exact standalone fixture for the native compiler without executing Python-side compilation or tests.

Native validation remains pending. Parent can freeze this worktree at `7cba9585` and build `intel_extreme.driver` using the existing coordinated native pipeline. No accelerant/GART/ABI rebuild is required by this patch alone, although building the normal complete component bundle remains useful integration coverage. A hardware smoke test should retain working desktop/SSH/HDMI; synthetic user-copy faults can first be exercised in an isolated VM, without probing new GPU command execution.

## Initial architecture findings

Current native Alder Lake support is display-only: accelerant engine count is zero, legacy ring allocation is skipped, and no modern engine submission path exists. Working scanout does not establish GPU rendering or 2D acceleration.

The native `intel_gtt.h` helpers validate 64-bit GGTT range, alignment, reserved aperture boundaries and the initial ADL-P/N 39-bit DMA limit. Firmware/stolen scanout mappings are preserved. System-memory binding is mediated by GART and its tracked partial-binding rollback. The shared framebuffer, raw aperture addresses and trusted accelerant MMIO are not an application-facing managed-buffer interface.

Important distinction: Linux `intel_ggtt.c`, pinned commit `f66469d9905b39d2c260a15d7d6db450f4db67c1`, lines 201–229 maps ICL+ GGTT uncached and deliberately omits the legacy `GFX_FLSH_CNTL` write in `gen8_ggtt_invalidate()` for that mapping. This does **not** justify adding guessed invalidation/forcewake writes to RenkuOS. Existing `wbinvd` cost and lifetime ordering require a separate review before optimization.

Primary reference: [Linux Intel GGTT implementation](https://github.com/torvalds/linux/blob/f66469d9905b39d2c260a15d7d6db450f4db67c1/drivers/gpu/drm/i915/gt/intel_ggtt.c).

Remaining prerequisites before executing modern GPU workloads:

- Kernel-owned buffer objects with non-address handles, ownership, bounds, pinning and completion references; current allocation/free addresses do not provide those properties.
- A supported GT power/wake lifecycle and engine discovery distinct from display power.
- A proven execution/context mechanism, interrupt/completion tracking, bounded waits and hang recovery.
- Validated command/surface addresses and formats; inherited legacy BLT opcodes cannot be assumed valid for Gen12.
- Measured hardware execution and CPU/software baselines before any acceleration claim.

Next audit is checking loss of allocation bounds at the accelerant's `size_t` to 32-bit ioctl boundary and defining the smallest managed-buffer ownership interface without enabling execution.

## Second checkpoint: exact allocation size

Commit `1fee8aaa` rejects zero and `size_t` requests greater than `UINT32_MAX` in accelerant `memory.cpp` before assigning the 32-bit ioctl field. It preserves the caller's output base on rejection. This prevents large requests aliasing a smaller backing allocation.

`memory-size-host.py` exercises the actual function with 111 sanitized boundary/failure cases, including 100 oversized values with nonzero low bits. The baseline fails 104 assertions. `--emit-cpp PATH` emits the standalone native fixture. Platform status macros are preserved when present, avoiding conflicts with Haiku `errno.h`; host compilation also passes when preexisting Haiku-style status macros are supplied.

Parent reported native driver compilation succeeded; complete native fixture validation remains with the parent. No hardware acceleration or managed-buffer support is claimed.

## Third checkpoint: preserve retained Intel aperture state

Commit `50691f19` adds a private Intel backend singleton-ownership flag. If GART unmap fails, the old registry object, allocations, mappings and module reference survive. Previously a normal reopen could overwrite that retained singleton and initialize its GGTT again. A second creation now returns `B_BUSY` before changing any state, mapping or PTE. Ownership is published only after complete initialization and cleared after deletion; failed initial map/PTE initialization remains retryable. The shared ABI, legacy graphics callback cookie and managed API are unchanged.

Validation:

- Ten tests execute actual Intel create/delete function bodies, with only hardware/mapping callbacks and the privileged cache instruction mocked. They cover initial failure, partial PTE setup failure, rejection without side effects, later cleanup/recreation, unrelated PCI devices and repeated deletion. Sanitizers pass; the old source fails seven assertions.
- A separate test executes actual generic GART map/unmap bodies against a simulated singleton backend. It verifies retained-ID retry, module references and global/per-aperture lock coverage; its guarded-backend mode rejects reopen and permits recreation after cleanup. Unguarded mode reproduces the old duplicate-backend creation hazard.
- Native compilation is pending for `intel_gart`; the parent coordinates it. Recovery/adoption of a retained aperture ID remains separate work. No modern GPU commands or hardware rendering are enabled.

```sh
python3 gpu-driver/workstreams/gpu-next/intel-singleton-aperture-host.py source/gpu-next-worktree/src/add-ons/kernel/busses/agp_gart/intel_gart.cpp
python3 gpu-driver/workstreams/gpu-next/gart-retained-aperture-host.py source/gpu-next-worktree/src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp --guarded-backend
```

Both generators support `--emit-cpp PATH` for native compiler validation. The manager test models locking assertions and references; it is not a concurrent kernel stress test or actual module-unload test.

## Fourth checkpoint: manager metadata allocation failure

Commit `f01e0b78` checks the nothrow `Aperture` allocation in generic GART `map_aperture()`. If it fails after backend creation, the manager deletes the new backend while still holding its module reference, drops that reference, and returns `B_NO_MEMORY` with no registry object. This is six production lines; no callback, ownership-recovery API or submission change.

The actual-manager fixture now injects class-specific metadata allocation failure and metadata initialization failure, with strict backend-delete/module-reference ordering checks. Its guarded-backend mode passes 13 cases under ASAN/UBSAN with exceptions and RTTI disabled. Running it against the prior manager reproduces a sanitizer-detected null-pointer member access on metadata OOM. Native compilation is pending for `agp_gart`; the parent owns that build window.

## Parent native validation: build 062

All four checkpoints above now compile in the matching RenkuOS x86_64 build
VM. The native fixtures pass 116 allocation-ioctl cases, 111 allocation-size
cases, 10 Intel singleton cases, and 13 generic GART lifecycle/OOM cases.
The host finalizer verifies the source manifest, linked module formats and
expected dependencies for intel_gart, agp_gart, intel_extreme and its accelerant.
This supersedes the pending-native notes above. These memory changes have not
been installed on the physical laptop, and no command submission or rendering
acceleration is enabled. The physical laptop remains on GPU cleanup build 057.

## Continuation: 2026-11-07 cleanup diagnostics

Revalidated the 250 existing host memory/GART cases successfully. Traced
last device_free through intel_extreme_uninit and uninit_driver: the graphics
structure is freed at module unload, while generic GART retains its registry
and backend reference on failed DeleteAllMemory. A graphics-only retained ID
would not establish recovery across unload. No recovery API was added.

Working source change centralizes the three ignored unmap results in
release_aperture(), logging aperture ID, actual status and GART retention on
failure. Success is quiet. Initialization unwind and last-close teardown use
the same helper. No register writes, ABI, teardown order or backing ownership
changed. This is an observability prerequisite, not an ownership recovery fix.

aperture-cleanup-host.py extracts the production helper and checks five
success/error cases under ASAN/UBSAN, plus all three call sites. All pass.
Source diff whitespace check passes. Native compilation and hardware checks
have not been performed for this change. Physical baseline remains build 057.

Run from renkuos-development:
python3 gpu-driver/workstreams/gpu-next/aperture-cleanup-host.py source/gpu-next-worktree/src/add-ons/kernel/drivers/graphics/intel_extreme/intel_extreme.cpp

## Native preparation checkpoint: renku-063

Exported diagnostic change as patch 0069 and prepared fresh bundle
build/renku-063 using prepare.py against pinned source plus patches 1–69.
check-source.py passed all source checks and 255 payload hashes. Exported
intel_extreme.cpp exactly matches the working source. Five production-helper
ASAN/UBSAN cases also pass against that exported source.

Native execution remains blocked on locating the authoritative Fedora
coordination host: workstation Qwen runs Ubuntu and its mirrored coordination
directory lacks TASKS.md and bin/coord.py. No guest was started and no resource
lock domain was substituted. Previous private build 040 was inspected only.

Recovery design finding: generic Aperture stores backend pointer and ID, but
no PCI bus/device/function identity or explicit retired/cleanup-only state.
Automatic recovery on map must not indiscriminately delete an active aperture.
A future manager-owned retry path needs an explicit failed-unmap state and
stable device identity under sLock, with aperture locking during cleanup.
That would survive graphics-driver unload because GART owns retained records.
A graphics-only retry ID cannot provide that cross-unload guarantee. This is
a design direction, not implemented or validated recovery behavior.

## Local native validation completed: renku-063

User explicitly selected the existing local QEMU workflow, superseding the
older Fedora-only location requirement for this operation. Held local canonical
host-config, haiku-vm and build locks for preparation, guest execution and
cleanup. No preexisting QEMU process was present. Created fresh private build
041 with a reflink/sparse copy of stopped build 040 disk and copied UEFI vars;
the prior disk and recovery artifacts were preserved.

Bundle 063 compiled with matching hrev60072+518 development package and GCC
13.3.0. tools/run-native.sh exited 0, and host finalize-native.py exited 0:
source/payload verification, native tests and linked controls, ELF formats,
module dependencies and binary hashes passed. VM shut down cleanly with exit 0.
The new five-case helper fault test was host-only; it was not separately run
as a native fixture. No hardware installation or GPU execution occurred.

Evidence: installation/private/gpu-hdmi-build-041/native.log and validation/;
source bundle: gpu-driver/build/renku-063. Recovery implementation remains open.

## Retained-aperture recovery: build 067

Production manager now records PCI bus/device/function and a cleanup-pending
flag per aperture. Failed unmap marks that flag under the global manager and
aperture locks. A later map for the same PCI identity retries DeleteAllMemory
before backend creation. Failure returns the cleanup error and retains the
registry/backing/module reference; success removes the retired record and
allows fresh creation. Active records and other PCI identities are untouched.
This makes recovery independent of graphics-driver-local retained IDs.

The scan uses RemoveUnchecked: normal hash removal can resize and invalidate
an iterator. No backend ABI, graphics ioctl, GGTT programming or submission
interface changed. Exported as patch 0070. These are uncommitted working changes.

Validation: gart-recovery-host.py --guarded-backend passes 17 production map/
unmap control-flow cases under ASAN/UBSAN with mocked backend/record methods.
Checks cover active records, differing bus/device/function, continuing cleanup
failure, automatic successful retry, metadata OOM and module/lock accounting.
The existing host lifecycle fixture passes 154 current checks; original control
reproduces 39 expected failures. Its mock now supplies cleanup-state marking,
with the extra field at the end to preserve aggregate initialization.

Native bundle 067 passes compilation, bundled tests and the separate 17-case
recovery fixture on the matching hrev60072+518 SDK. Host finalizer passes all
256 payload hashes, source checks, ELF/dependency checks and artifact hashes.
Local canonical locks cover guest preparation, execution and cleanup. VM shuts
down cleanly. Evidence: installation/private/gpu-hdmi-build-045/native.log,
finalize.log and validation/. No physical installation or GPU execution.

Preserved attempts: 064/build-042 never reached SSH readiness and was stopped
via its QMP socket; 065/build-043 failed because the existing lifecycle mock
lacked MarkCleanupPending; 066/build-044 exposed shifted aggregate initialization
in that fixture. Build 067 resolves those fixture issues. Original known-good
build-040 disk used for fresh copies; previous disks preserved.

Limits: mocked callbacks are not concurrent kernel stress or real unload/
reload validation. Reopen now treats a prior failed unmap as pending teardown;
callers must not treat such an aperture as active while requesting a new map.
Review all GART clients and hardware retry behavior before deployment. No
managed-buffer API or command execution has been added.

## Maintainability and lifetime audit: builds 068/069

Found and closed an aperture pointer lifetime race: all seven manager callbacks
that use get_aperture now hold sLock from lookup through last use, preserving
manager-then-aperture lock order. Host guard/query tests pass; existing 154-case
lifecycle suite passes. Native build 068, bundled tests, 17 recovery fixture
cases and host artifact finalizer pass; disposable VM exits cleanly. Evidence:
installation/private/gpu-hdmi-build-046/native.log, finalize.log and validation/.
No hardware driver installation or concurrent kernel stress test occurred.

At user request, reviewed maintainability and split the previously combined
manager diff into patch 0070 (recovery) and 0071 (callback lifetime). Bundle 069
applies the split patches successfully; its agp_gart.cpp is byte-identical to
the natively tested build 068 source. Bundle 069 source/255 payload checks pass;
no separate native 069 run is claimed. Build 068 preserves its original patch
hashes, while the current patch series is the split form.

MAINTENANCE.md now records ownership, failure retention, recovery eligibility,
lock order, callback non-reentrancy, safe iterator removal and validation limits.
Review broader caller expectations and add concurrent lifetime stress coverage
before physical deployment or reducing manager serialization. Goal remains open.

## Threaded host lifetime regression

Added workstreams/gpu-next/gart-threaded-host.py. It extracts production
get_aperture_info and unmap_aperture bodies, uses real host mutexes/threads,
and gates a query while teardown attempts the manager lock. All 100 rounds
pass ASAN/UBSAN; the test verifies no destruction during query, successful
teardown afterwards and missing-ID behavior after deletion. Removing callback
manager guards in a temporary negative-control source causes a bounded gate
assertion as expected. Backend/registry/aperture services are mocked; this is
not a concurrent kernel/hardware stress claim. No native source changed.

At user request, wrote Desktop/RenkuOS-GPU-status.txt and updated it with this
host result. Development goal remains active.

## Checked allocation rounding: build 070

Generic allocate_memory rounded size_t before validating it, allowing values
near SIZE_MAX to wrap. Patch 0072 rejects zero and sizes exceeding the maximum
page-roundable value before lookup or allocation. Output addresses remain
untouched on rejection. No ABI or new helper added.

gart-size-host.py executes the full production allocation callback with mocked
services: nine zero/overflow/valid rounding cases and flag/null/missing-ID
checks pass ASAN/UBSAN. Previous bundle 069 fails the regression as expected.
The new size fixture is host-only. Native bundle 070 passes compilation,
bundled tests, separate 17 recovery cases and artifact/provenance finalization.
Evidence: installation/private/gpu-hdmi-build-047/{native.log,finalize.log,validation}.
VM shuts down cleanly under canonical local locks. No physical installation.
Desktop status updated. Managed-buffer design remains open; this is a checked
backend boundary prerequisite rather than command submission or acceleration.

## N100 execution mission design

User specified physical command execution as the immediate mission and requested
its smallest safe technical design. Committed project documentation 70908c2
with Signed-off-by and Assisted-by trailers: research/n100-submission/DESIGN.md,
REVIEW.md and references.json. Verified 13 downloaded pinned Linux source hashes.
Initial MOCS fetch returned HTTP 429 and remains an explicit research gap.
Design proposes conditional execlists/BCS0, private VM/context and a kernel-built
nonce store with ordered completion/retirement; it is not an implemented path.
Recorded ADL-N's firmware selection exception and separate review gates.

Prior in-flight build 071 finished: native compilation, bundled tests, 17 recovery
cases and host finalizer pass; VM cleanly shut down. New placement test has 816
host cases and fails against prior source, but is not a native fixture result.
Evidence: installation/private/gpu-hdmi-build-048/native.log,finalize.log,validation/.
No physical hardware tests or driver installation performed this turn.
Goal remains active. Next mission work is owned DMA/module lifetime and complete
GART caller contract review before read-only GT inventory or any workload.

## Mission Phase 1: caller contract and retired-state admission

Audited complete Git HEAD blobs, not only the sparse filesystem. Intel Extreme
is the production aperture callback caller; gart_tester is the test caller.
Other AGP module consumers do not invoke aperture callbacks in the searched
in-tree graphics sources. External binary clients remain outside this evidence.
See research/n100-submission/GART-CALLERS.md.

Patch 0074 makes failed-unmap records cleanup-only: allocate/bind return B_BUSY
under manager locking; query and explicit free/unbind/unmap remain available.
Host allocation regression verifies untouched outputs/no allocation. Existing
lifecycle fixture now asserts pending state and bind rejection after unmap fault.
154 lifecycle checks and original regression control pass. Native build 072,
bundled tests, 17 recovery cases and artifact finalizer pass; VM shut down cleanly.
Evidence: installation/private/gpu-hdmi-build-049/. No physical tests or workload.
Module lifetime and backend partial-bind orphan recovery remain next gates.

## Terra R2: transactional hash publication

Patch 0076 checks registry and allocation hash insertion failure. Aperture ID/base
is not published until registration succeeds. Memory metadata is unlinked and
destroyed if hashing fails before backing allocation. Existing caller APIs unchanged.
18 manager cases pass host sanitizers and native fixture, including registry OOM;
100 failed memory-record publications plus successful retry pass host-only fixture.
Both updated fixtures reject previous build 073 source. Tests inject Insert's
failure return; actual kernel initial-bucket OOM is not claimed.
Native build 074, bundled tests, real-semaphore query regression and finalizer pass.
VM clean shutdown; evidence installation/private/gpu-hdmi-build-051/.
Terra independent review requested through CODE_REVIEW.md. No physical deployment.

## R3 aperture semaphore cleanup: build 075

Patch 0077 deletes valid aperture semaphores before backend/module release;
failed lock initialization is excluded. Actual-destructor host test passes 200
success/failure cases with mocked semaphore/backend operations. Previous source
fails runtime negative control. Native compilation, bundled tests, 18 recovery
cases, 2 actual-lock query cases and finalizer pass. VM shut down cleanly.
Evidence installation/private/gpu-hdmi-build-052/. No physical validation.
Manager teardown/init unwind remains open with R4; R3 is only partially resolved.
Terra independent validation requested in coordination/CODE_REVIEW.md.

## Terra R4: safe manager teardown iteration

Patch 0078 uses RemoveUnchecked during agp_uninit iteration. New host test uses
actual production teardown, OpenHashTable and TypeOperation headers. Expanded
256-object registry cleanup and failure at object 128 followed by retry pass
ASAN/UBSAN. Previous build 075 source fails the same regression. OS/backend
cleanup is mocked; no real kernel module teardown is claimed.
Native build 076, bundled tests, 18 recovery cases, 2 real-lock queries and
finalizer pass; disposable VM shuts down cleanly. Evidence build-053 under
installation/private. R4 awaits independent Terra validation. Manager R3 and
borrowed backing R5 remain unresolved. Desktop/coordination records updated.

## Manager resource lifetime, build 077

Patch 0079 balances PCI on init-lock failure, releases manager semaphore and hash
storage only after complete teardown, and leaves an empty valid global table for
later initialization/static destruction. Failed cleanup retains all manager
resources for retry. Extended gart-uninit-host.py covers real hash and production
init/uninit with mocked OS/backend; old 076 fails runtime control. Native 077,
bundled tests, 18 recovery cases, 2 native real-lock queries and finalizer pass.
VM clean shutdown; evidence installation/private/gpu-hdmi-build-054/.
Terra review requested; no physical deployment. R5 and execution ownership remain.

### Build 078 — memory-only diagnostic preparation

Implemented physical/memory-smoke.cpp: 30 serial owned non-reserved allocation/free
cycles with exact returned descriptors, stop on failure and a 20-second user alarm.
The alarm cannot interrupt a stuck kernel ioctl; verified external recovery remains
mandatory. Procedure physical/MEMORY_VALIDATION.md awaits Terra review.

Native compilation and bundled fixtures pass in the isolated RenkuOS QEMU VM,
along with 18 recovery cases and 2 actual semaphore-lock query cases. Diagnostic
compiles with -Wall -Wextra -Werror; only --help ran, opening no graphics device.
First diagnostic compile failed from missing graphics/common include directory;
corrected command passed. Both attempts preserved. Finalizer passes on the second
attempt's evidence, validation-attempt-2; first-attempt evidence stays in validation.
Evidence: installation/private/gpu-hdmi-build-055/{native.log,run.py,
validation-attempt-2,finalize-attempt-2.log}; initial failure native.log.attempt-1.
VM shut down cleanly. Production source unchanged from 077. No driver module load,
physical test, diagnostic allocation path test or GPU workload execution is claimed.

Next gate: independent R3/R4 and procedure review; verified live recovery/known-good
boot path before physical allocation smoke. R5 borrowed backing and execution-owner
design remain unresolved; no GPU publication permitted.

### Expanded deterministic callback lifetime coverage — 2026-11-07

Astra extended gart-threaded-host.py to extract actual allocate_memory and
free_memory callbacks alongside get_aperture/get_aperture_info/unmap_aperture.
300 total rounds (100 per operation) pass ASAN/UBSAN with real host mutexes and
threads. A controlled backend pause requires concurrent unmap to block on the
manager lock before allowing completion; outputs and post-unmap missing-ID results
are checked. Aperture/backend/page services remain modeled. This does not prove
real kernel allocator, bind, GPU DMA or module-unload behavior.

Removing each callback's manager guard independently fails the runtime blocker
assertion. Initial negative run failed the backend deadline assertion first;
separated deadlines (backend 10 seconds, observer 5) make the intended failure
identifiable. Preserve that initial observation; it was not a positive pass.
Evidence: testing/gart-threaded-300/{result.json,positive.log,*-negative.log,
verify.py}; source/test SHA-256 recorded. No production source change, no new
native build (latest 078), no physical test or GPU execution. Bind/unbind/reopen
and actual concurrent kernel stress remain incomplete.

### Build 079 — isolated memory-owner lease bookkeeping

Patch 0080 adds IntelApertureLeaseState.h beside the driver: fixed display/request
leases, nonreused numeric IDs, operation pins through client release, allocation
failure diagnostic blocking before unpin, retained display, single final-cleanup
winner and failed-cleanup quarantine. All methods require the containing owner's
mutex. There is no backend call, hardware command, module retention implementation
or driver wiring yet. It cannot independently establish quiescence or safe DMA.

Host tests compile with -Wall -Wextra -Werror and ASAN/UBSAN; transition cases and
100 close/operation races pass. Three separate negative variants (removed pin
check, removed failure block, reused IDs) fail runtime assertions. Evidence
 testing/intel-owner-001/{result.json,verify.py,*/test.log} includes hashes.
Build 079 compiles full native bundle and passes existing component tests,
18 recovery cases, 2 real-lock queries, diagnostic help-only and new lease state
harness in the RenkuOS VM. Its printed host-state label means state-only modeled
harness, not a kernel/module test. Finalizer passed; VM shut down cleanly.
Native evidence: installation/private/gpu-hdmi-build-056/{native.log,finalize.log,
validation/result.json}. No physical test or real GPU execution.

Next: Terra implementation review, then retained-module GART/MMIO adapter and all
N100 driver paths. Native module-load/clone/lifetime tests and physical memory
validation still required. Saved physical target remains unreachable.

### Build 080 — reviewed lease defects corrected

Implemented patch 0081: pinned display operations admitted during initialization;
finish cannot pass active tickets; allocation failure remains sticky and prohibits
final cleanup of potentially hidden backing. Corrected prior test expectation.
Host sanitizer tests pass, including initialization outcomes, display/request
failures and 100 close races. Old 0080 plus 3 targeted broken variants fail runtime.
Native 080 compiled and passed component fixtures, 18 recovery cases, 2 actual-lock
queries, help-only diagnostic and updated userland lease harness. Finalizer passes;
VM clean shutdown. Evidence installation/private/gpu-hdmi-build-057/ and
 testing/intel-owner-002/. No kernel module-load, physical test or GPU execution.
Independent Terra validation pending. Retained GART/MMIO module/driver adapter
remains unimplemented; hardware access and memory-safety gates remain required.

### Builds 081–083 — retained memory-owner module, not driver-wired

0082 adds a separate B_KEEP_LOADED intel_memory_owner module with own PCI/GART
references, validated N100 identity, private numeric display lease, retained MMIO
mapping and pinned query/allocate/free forwarding. Lease core moved to private
shared graphics headers. No request/submission/recovery interface or driver wiring.
0083 fixes Terra's failed-free finding: Allocate OR Free failure sets persistent
unresolved backing before RAII operation unpin. Successful later free does not
clear it. Display query/free/allocation remain permitted; diagnostics and final
cleanup remain blocked. MMIO BAR validation rejects I/O, zero, alignment/overflow
errors. Mapping failure retains this boot's owner state even if no resources were
acquired, with no teardown of another owner's aperture. Existing GART MMIO alias
is separate; this is not a single-global-alias claim.

Host actual-module fixture passes with ASAN/UBSAN and modeled OS/PCI/GART services;
focused missing-free-failure control fails runtime. Terra independently reran it
and accepts 0083 source/host correction. Evidence testing/intel-owner-adapter-001/.
Serial faults/forwarding only; concurrent backend, module-load, clones/IRQ and
physical cases remain outstanding.

081 compiled module, but collector omitted its artifact. Strengthened collector
and verifier: owner source requires collected owner ELF, modules entry and only
_KERNEL_ dependency. Old081 archive is rejected for missing owner;082 verifies it.
083 compiles latest correction, passes native component/recovery/real-lock/state
fixtures, diagnostic help-only, full finalizer and clean VM shutdown. Evidence
installation/private/gpu-hdmi-build-060/{native.log,finalize.log,validation/result.json}.
No kernel module loaded and no physical GPU execution or deployment occurred.
Build/collector changes are needed to verify the new module; no unrelated build
infrastructure added. Full OS Jam build remains untested.

Next: adapter pin-versus-release with faulting backend, then reviewed N100 driver
wiring, error/clone/IRQ ownership tests and native kernel validation. Saved target
SSH remains unreachable; physical admission and GPU cache/DMA/TLB gates stay open.

### Build 084 — staged N100 driver wiring and adapter concurrency

Actual adapter fixture expanded: 300 query/allocate/free backend-barrier races
against display release, success/failure, and initializing finish/release barriers.
Host ASAN/UBSAN passes; premature/unreleased pin controls fail runtime. Terra
independently ran positive concurrency suite. Evidence testing/intel-owner-adapter-002/.
OS/backend/modules are modeled; this is not native kernel or hardware concurrency.

Patch0084 stages exact N100 rev0 routing to retained owner. Init acquires independent
owner resources, uses module-returned aperture info/MMIO, and finishes the lease
before reporting success. Allocate/free helpers forward through owner; unwind and
last close release the lease/client module ref without unmapping owner GART/MMIO.
Other GPU path remains legacy. Shared accelerant ABI is unchanged. Legacy display
IRQ removal stays before release; no module GT IRQ/submission exists. Native084
compiles full candidate, passes existing fixtures and artifact finalizer with clean
VM shutdown. Evidence installation/private/gpu-hdmi-build-061/. Four actual helper
routing cases, four retained-owner release cases and five legacy diagnostics pass
host sanitizers. These helper tests do not prove whole-driver init/uninit lifetime.

Terra preliminary source review found no additional unconditional failure but
requires full behavioral init/error/uninit, native clones/IRQ/module tests before
integration acceptance. Candidate is not deployed or accepted as physically safe.
Same-boot reopen after last close remains B_BUSY; normal desktop boot/clone flow
compatibility is unverified. No physical memory test or GPU execution.
Next: driver lifecycle fault harness and native VM clone/IRQ test specification.

### Full driver lifecycle host fixture — 2026-11-07

Added driver-lifecycle-host.py: extracts unchanged complete production init/uninit
and allocation/free/release functions. 36 N100/legacy, normal/fallback IRQ and
module/begin/shared/map/finish/allocation fault cases pass ASAN/UBSAN. Four targeted
runtime negative controls fail: deleting owner MMIO, remapping owner MMIO, omitted
IRQ removal and leaked caller module reference. Evidence testing/driver-lifecycle-001/.
Register constants are symbolic and OS/owner/IRQ services are modeled; this tests
lifetime control flow only. Does not prove native kernel, clones, IRQ draining,
MMIO correctness, scanout or physical safety. Initial harness compile errors in
stub types/constants were corrected before executing tests. Terra independently
ran the positive suite and confirmed its scoped results.

Terra identified a conditional finish-B_BUSY unwind gap: repeated busy finish and
release can leave a live initializing lease after driver pointer/reference loss.
Module retention prevents demonstrated premature free, but actionable lifecycle
state needs resolution. Proposed explicit initializing-lease revocation retaining
operation pins and all backing, pending independent design disposition. Candidate
wiring acceptance is NOT complete. No production change this checkpoint, no new
native build (latest084), no physical test or GPU execution.

### Build 085 — initializing lease revocation and native RAM clones

0085 adds explicit AbortInitialization; module release revokes a matching live
initializing display lease when normal release is Busy. Existing operations retain
pins, reject new/stale calls, and record late failed allocation/free before unpin.
No hardware idle, cleanup or reopen success implied. This closes Terra's conditional
orphaned-live-lease finding at source/host scope, independently validated by Terra.

Host state/adapter/driver suites pass; old release and two broken abort variants
fail runtime. Evidence testing/intel-owner-abort-001/. Driver fixture now models
successful revocation and asserts no live caller lease survives finish-B_BUSY.
Native085 compiles candidate and passes component fixtures, 18 recovery cases,
2 real-semaphore queries, updated lease harness, diagnostic help-only and finalizer.
Terra's same-team ordinary-RAM clone probe compiled/runs on actual RenkuOS in VM:
remaining clones preserve visibility after original and first clone deletion.
This is real native area-API behavior, not GPU MMIO, cross-team/kernel-area cloning
or execution evidence. VM clean shutdown. Evidence installation/private/gpu-hdmi-build-062/.
No physical GPU test/deployment and no kernel driver/module load in this build.

Next: isolated native kernel module-load/rejection and kernel-RAM clone fixture;
Terra prepares source, Astra reviews/runs in disposable VM. Real IRQ drain, normal
N100 boot/reopen compatibility, physical memory validation and DMA/cache/TLB/reset
requirements remain open. Full-driver integration acceptance is still incomplete.

## Astra: native086 kernel owner/RAM probe — 2026-11-07

Implemented: isolated test driver/client and auxiliary Makefile; production0085
candidate unchanged. Native compilation, component fixtures and finalizer passed.
Actual owner kernel module loaded; impossible PCI BDF rejected with unchanged
outputs. User RAM clone survived original kernel area deletion, checked explicitly
with get_area_info. Disposable VM shut down cleanly. Owner SHA-256 matches the
finalized artifact: 592fca23f7b96b289ae9e602a3d8f08d572dbd157285255603946d33a4975b10.
Evidence: /home/dan/renkuos-development/installation/private/gpu-hdmi-build-064/
(native.log, probe-result.json, probe-hashes.json, kernel-probe.log, finalize.log).
Attempt063 client passed but runner failed decoding binary syslog; it is incomplete.
Fresh repeat064 captured bytes and completed cleanly. The finalizer's component
scope excludes module loading; separate probe evidence establishes this result.
No new host simulation. No N100 BAR, real IRQ, physical test or GPU workload.
IRQ acquisition/unwind defects, desktop/MMIO clone compatibility, retained-owner
reopen, borrowed-buffer ownership and physical recovery remain unresolved.
Terra independent evidence review requested. Next: actual IRQ setup fault tests
and explicit resource ownership; no production interrupt fix claimed yet.

## Astra: actual IRQ initializer baseline fixture — 2026-11-07

Added uncommitted `workstreams/gpu-next/irq-init-host.py`. It extracts the complete
production init_interrupt_handler and models only external APIs/register accesses.
16 cases cover missing/valid legacy vectors and semaphore creation, caller lookup,
owner transfer, MSI configuration/enable and handler install failures.
Host compilation passes. Runtime deliberately exits2: 12 cases have incomplete
MSI resource or fallback state under the fixture predicates. Evidence:
`/home/dan/renkuos-development/testing/irq-init-001/baseline.log` and exit-status.txt.
This is a failing baseline, not a successful regression suite or physical IRQ test.
No production correction yet; init/uninit ordering, semaphore-zero and failed
teardown retention still need coverage. Terra review requested before correction.
Native086 fixture checkpoint committed as8057b93; no source worktree commit.

## Astra: IRQ admission correction and teardown scope — 2026-11-07

Production candidate patch0086 initializes IRQ fallback/vector/MSI state before
semaphore acquisition, returns before PCI work when semaphore owner setup fails,
sets error explicitly when no vector exists, and accepts valid semaphore ID0 for
normal handler teardown. It does not solve partial MSI cleanup or semaphore lifetime.

Expanded irq-init-host.py extracts complete actual init AND uninit functions.
Focused --early-paths checks pass on candidate and fail on pre0086 snapshot.
Three targeted reverted changes fail runtime checks (testing/irq-init-002/controls.json).
Whole-driver modeled lifecycle still passes36 cases. Full IRQ probe remains a
FAILING baseline:12 of24 cases flag resource/fallback issues, including successful
teardown's live semaphore, injected MSI cleanup failures and handler-removal failure.
These predicates describe modeled safety requirements, not observed hardware faults.
Native build087 started in fresh VM065 with canonical locks; result pending.
No physical test, GPU workload or changed known-working boot configuration.

Pinned source audit (source HEAD f01e0b782747076cb18d6c9f7952bcc358ee87e8):
PCI::ConfigureMSI reserves vectors separately; EnableMSI returns errors before
its register writes. DisableMSI clears enable but does not free vectors.
UnconfigureMSI frees vectors without first disabling MSI. Therefore a failed disable
must prevent unconfigure; configured/enabled state must be tracked independently.
Sources: src/add-ons/kernel/bus_managers/pci/pci.cpp:2193,2240,2271,2306.
Current kernel removal returns B_BAD_VALUE for an invalid vector or absent matching
handler, B_OK for successful removal under vector_lock (interrupts.cpp:524).
The injected failure with a still-live handler is a stronger adversarial contract,
not a reproduced reachable backend failure. Normal installed-handler invariants
and no_lock_vector dispatch semantics still require proof.

Important design constraint: legacy uninit_driver frees intel_info and unloads
callback code unconditionally; merely returning early from intel_extreme_uninit
would not establish safe retention across driver unload. B_KEEP_DRIVER_LOADED is
for device-manager nodes and cannot be assumed to apply to this legacy driver.
Do not claim a local teardown guard solves callback code lifetime. Terra review
requested for scoped0086 and larger IRQ ownership contract before integration.

### Build087 native result

Native compilation and component fixtures passed; artifact finalizer passed.
VM065 shut down cleanly. Candidate driver source byte-matches exported bundle.
Evidence: installation/private/gpu-hdmi-build-065/native.log and validation/result.json.
Driver SHA-256:410dd1560961fd38b560546be1f35ec85f2b27d3187b23af6c13380f5a9107bd.
This run did not install/test the candidate graphics driver, exercise actual IRQs,
or perform physical GPU execution. Prior086 kernel probe remains separate evidence.
0086 independent review requested, not integration acceptance. Remaining full IRQ
fixture failures are intentionally preserved and must be resolved before safety
approval. Known-working057 hardware boot configuration remains untouched.

## Astra: explicit MSI configuration cleanup candidate0087

Accepted Terra0086 scoped review; native087 evidence remains passed.
Candidate0087 separates msi_configured (vector reservation) from use_msi (enabled).
release_msi checks disable before unconfigure, clears flags only on success, and
preserves state on error. Failed enable cleans up reserved vectors before legacy
fallback; successful enable followed by failed handler setup cleans MSI but does
not attempt INTx fallback (PCI interrupt-disable command bit is not restored).
Uninit performs MSI cleanup independently of fake interrupt/semaphore state.
Full driver init rejects retained MSI configuration before allocating resources.
This is same-instance protection, not a durable recovery record across driver unload.

Actual extracted initializer/uninit/helper host MSI checks pass, including failure
of disable/unconfigure during init unwind and successful later retry. Three broken
variants fail runtime checks: forgotten configured flag, ignored disable error,
and omitted failed-init MSI cleanup. Source/tests/logs: testing/irq-msi-001/.
Whole-driver36 cases pass; an added pre-allocation retained-MSI check passes.
Full IRQ baseline still reports8 cases: semaphore lifetime, injected callback
removal failure, and retained MSI resources. These are not full acceptance criteria
met. Counts exclude four extra acquisition-cleanup retry checks. No new physical test.

Build088 exported byte-matching source; native build underway in fresh VM066 under
canonical locks. No candidate display driver installed. Terra independent review
requested. Next gates remain callback/code lifetime and semaphore ownership,
MMIO/desktop compatibility, controlled physical memory validation and recovery.

### Astra0087 disposition and native088 result

Accepted Terra's independent scoped MSI review and reproduced host results.
Native088 compilation, component fixtures and artifact finalizer pass; VM066 shut
down cleanly. Exported driver source matches candidate. Driver SHA-256: cd0e08af586c45c2054d4ca328226e53c141e2259c7a3057f7796b5b8d0e5610.
Evidence: installation/private/gpu-hdmi-build-066/native.log, finalize.log,
validation/result.json. Host generation4/8/12 branches pass MSI requirements;
three negative controls detect broken ownership/cleanup. No actual IRQ tested.
No hardware test or GPU execution, candidate driver not installed in physical target.
Full IRQ lifecycle remains incomplete: callback/code lifetime, semaphore ownership,
and retained-resource recovery still gate integration/hardware submission.

Next semaphore requirement: accelerant.cpp:127 clones shared_info READ|WRITE;
release_vblank_sem and teardown currently trust its vblank_sem. Keep a private
kernel-owned semaphore ID before adding normal cleanup; otherwise client mutation
could redirect deletion or IRQ signaling. sem.cpp delete_sem_internal checks full
ID, removes team ownership and wakes waiters; delete_sem uses kernel privileges.
Team exit can invalidate the ID first. Test concurrent owner exit and explicit
cleanup and never infer handler ownership from this user-writable field.
B_NO_LOCK_VECTOR audit finds only x86 SMP/APIC/HPET callers in pinned source;
install_io_interrupt_handler documents nonsharing but does not enforce it.
Allocator/vector isolation and callback drain evidence remain required.

## Astra0088 private interrupt ownership checkpoint

Private intel_info now owns vblank_sem and interrupt_handler_installed. Shared ABI
is unchanged; shared_info semaphore is a published mirror, never the kernel's
signaling/teardown authority. Install flag is set after success, cleared only on
successful removal; new init rejects a leftover installed handler before allocation.
No normal semaphore deletion or failed-removal containment is added in this patch.
Source/host review accepted by Terra. Actual extracted signal/init/uninit functions
pass shared-ID tampering cases; restored shared signaling/predicate controls fail.
Early/MSI/private modes pass across generation4/8/12 modeled services. Whole-driver
36 cases pass. Evidence: testing/irq-private-001/results.json and logs.
Native089 currently running in fresh VM067; no physical GPU test or deployment.

This patch remains insufficient for callback safety: unsuccessful removal still
reaches legacy teardown. It must not be deployed as a completed safe lifetime fix.
Terra conditionally supports a development-only fail-stop for an invariant violation
before unsafe freeing, but requires exact install ownership and vector-lock proof.
Long-term production recovery needs a supported callback/data/code lifetime owner.
Terra assigned isolated kernel dispatch/removal probe design (no hardware IRQ).

Semaphore audit: kernel Team destructor puts io_context before deleting owned
semaphores (team.cpp:519-522), but exec deletes owned semaphores before processing
exec file descriptors (team.cpp:2033-2035); other teams can retain device handles.
Therefore owner-team teardown cannot be assumed to occur after final device close.
sem.cpp:346 increments slot IDs by sMaxSems; free_sem_slot resets a negative nextID
to the slot number (278-285). Exact-ID checks prevent ordinary immediate reuse but
are not a never-reuse proof. Avoid claiming stale-ID safety across arbitrary wrap.
A proper ownership/notification contract is still required for semaphore cleanup.
These are pinned source findings, not observed physical faults.

### Native089 result and scoped0088 acceptance

Compilation, native component fixtures and finalizer pass; VM067 shut down cleanly.
Candidate source byte-matches exported bundle. Driver SHA-256: 7eb88dcdcdd1fa5455b15628853c7f18268e2878afb5135586e32bf84b5836e2.
Evidence: installation/private/gpu-hdmi-build-067/native.log, finalize.log,
validation/result.json. Terra independently reviewed0088 source/host behavior.
Added full-driver retained-handler reinit rejection check passes before allocations.
No actual IRQ dispatch, new kernel probe, candidate driver installation or physical
GPU execution in this run. Full IRQ safety and normal semaphore cleanup remain open.
Next: native kernel dispatch/removal probe design and sem owner-exit lifetime contract.

## Astra: interrupt allocator exhaustion reproduced

Accepted Terra source finding; actual extracted allocator fails255/2048 exhaustive
host occupancy/request cases. Full eight-slot occupancy plus count1 wrongly returns
B_OK at already-owned vector0. Isolated one-line runFound=false candidate passes
2048/2048 and preserves outputs/storage on failure. Host compiled and executed;
no native kernel compilation/test or physical operation. GPU source unchanged.
Evidence testing/irq-vector-001/ and research/n100-submission/interrupt-vector-exhaustion.txt.
Candidate is outside production patches/, not installed. Independent review requested.
This is a direct x86 MSI allocation dependency, required for IRQ ownership proof;
full kernel build/provenance/native boot validation is separate from GPU component
builds. Synthetic IRQ stress remains held for allocator, export/architecture and
bounded SMP scheduling requirements. No claim current runtime kernel matches source.

## Astra: full-kernel validation path established

Accepted Terra independent allocator candidate review (2048 host cases pass).
Created isolated detached worktree source/gpu-kernel-validation at f01e0b78;
only source change is runFound initialization false. Active GPU worktree preserved.
This kernel change cannot be validated by the GPU component bundle.

Reused repository CI cross-compiler image, pinned after retrieval to
sha256:4500c502c2d5298a74ac79a593b82da59b6e14badbb4095c104a110db718d969
(ghcr.io/renkuos/cross-compiler, GCC13.3.0). Installed podman plus six dependencies
under host-config lock (95.1MB package estimate); disabled newly enabled podman
socket/services/restart/cleanup and auto-update timer after installation. No host
bridge or physical device passed to build container; network=host for packages.

Full kernel_x86_64 target started with four jobs, canonical build lock held.
Evidence testing/kernel-vector-build-001/build.log, build.sh and provenance.json.
Tracked exec session67688/container renku-gpu-kernel-vector-001. Completion pending.
Source checkout is shallow without reachable hrev tag, so build uses explicit label
git-f01e0b78-irq-vector-validation; do not equate it with a release or installed
runtime kernel revision. Configure and compilation started successfully.

No native kernel boot, IRQ dispatch or physical GPU execution this checkpoint.
Next: inspect completed link/export evidence, then reviewed disposable VM boot.
Terra researching alternate-kernel boot procedure, no deployment authority delegated.

### Full-kernel build completed

Full kernel_x86_64 cross-build exits0,864 targets updated. ELF64 x86-64 artifact
SHA-256:08c6d68220ee07b70ca5251458063bd53fc3a7eadfba2120d197394515e9f3c4.
Verified dynamic exports for allocator/free/install/remove/io_interrupt_handler;
these are candidate binary exports, not proof of installed guest ABI compatibility.
Evidence testing/kernel-vector-build-001/{build.log,result.json,kernel-elf.txt,
kernel-exports.txt,kernel_x86_64,provenance.json}. Build job terminal; locks released.
Committed only the allocator source correction in isolated branch
gpu/n100-vector-exhaustion: ee4f34defcd6aca262814ffdbc72cdb8f8af64b3. GPU worktree/source changes preserved.
No native boot, native IRQ dispatch, physical test or GPU workload. Next milestone
is reviewed disposable VM boot of this exact artifact, preserving recovery image.

## Astra: disposable kernel package/boot preparation

Built repository host BFS shell; offline package replacement/readback on copied040
image proven. Full6204 entry comparisons retain all non-kernel files, links, modes
and package metadata; kernel attributes present. Live system-package replacement
rejected per Terra review. Failed raw-kernel runtime-label attempt001 preserved;
corrected to project's revisioned boot target, with fresh002 retry underway.
See workstreams/gpu-next/kernel-boot-validation.txt for exact evidence and limits.
No physical GPU execution or native IRQ test. Runtime result pending.

## Astra: allocator-corrected kernel boot verified

Full cross-build and revision-stamped boot target passed. Controlled VM002 repeat
verified kernel-owned serial revision git-f01e0b78-irq-vector-validation, guest
kernel SHAe2fbc2f00d8bb4161dc994ed0950a96f4793861bd7a2a9173b8d699c9885bc21
and selected package SHA5a7885cd40b572b89d2bd818917bd51aada9f9201ebca835d1cfad57a40e9391.
VM shut down cleanly. Evidence installation/private/kernel-boot-002/verified-*.

Original package/kernel captured read-only from fresh040 copy; system package
replaced OFFLINE through repository BFS shell with explicit GPT bounds. Independent
remount/readback verified exact package and unchanged activation filename. Compared
6204 extracted entries and package metadata; only kernel entry changed. Recovery
source040 and physical057 boot configuration untouched.

Two earlier verifier failures are preserved: raw linkable kernel lacked test stamp;
then uname incorrectly used as kernel revision authority (it reads libroot revision).
Corrected to project's <revisioned> target and kernel-owned serial banner, retaining
exact input/guest hash checks. Initial failed extraction used read-only generic
attribute store; corrected packaging includes kernel build attributes.

Compiled: full kernel and host BFS/package tools. Host simulation: prior2048 allocator
cases/independent review. Native VM: boot and package/kernel identity, clean shutdown.
No new native IRQ stress, module-load probe or physical GPU workload.
Next safety gate: no_lock_vector can remain set after freeing a vector; allocation
exclusivity alone does not prove locked dispatch. Native IRQ probe still requires
supported vector state proof and bounded scheduling. Sem owner-exit lifetime and
controlled physical memory/recovery validation also remain open.

## Astra: native IRQ drain design prepared

Documented checked no-lock state, exact removal-entry observation, affinity via
user threads, independent callback deadline and full-dispatch resource retention.
Source review confirms affinity implementation in libgnu and kernel rejection of
kernel-team affinity requests. Terra independent design review requested.
Implemented: design only this checkpoint. Compiled/simulated/native VM: no new run.
Previous exact kernel boot evidence independently accepted by Terra. Physical: none.
Blockers: design review/instrumentation, failed-removal containment, semaphore
owner-exit lifetime and controlled physical memory/recovery validation.

Terra review resolved: replace entry-only overlap with observed failed try-lock
contention; revised design accepted. Confirm spinlock debug configuration, exact
run ID and single release in implementation. Code review still required before run.

## Astra: isolated IRQ instrumentation compiled and reviewed

Source commit 0dde7836 in gpu-kernel-validation adds opt-in test-only vector
checks, observed try-lock contention and 32 never-reused per-run records. Terra
review accepted atomic publication/spinlock semantics and the correction for
delayed readers across rounds. Caller serialization/worker lifetime is mandatory.
Instrumented kernel cross-links with seven expected exports. Forced disabled
rebuild cross-links with zero hook exports. Evidence testing/kernel-irq-build-002.
Initial optional debug-macro compile failure retained in build-001; corrected.
Existing array-bounds warning in IRQ CPU-routing branch appears in both builds.
No simulation, native runtime, IRQ workload or physical test this checkpoint.
Next: retained fixture and pinned user client, code review, then bounded VM run.
Physical memory validation, failed-removal containment and sem owner-exit remain
open. Regenerate enabled boot artifact in original build directory before package
creation to retain the build's external attributes; archived copy is hash evidence.

## Astra: resident fixture and pinned client implemented

Kernel commit 80cdd492: opt-in resident callback/state, one-open root admission,
serialized once-only workers, bounded waits, closing admission before cleanup.
Test commit d9507b4d: thin bridge and pinned client, sequential control plus ten
overlap rounds. Terra reviewed ordinary ioctl/free/unload lifetime, concurrent
close semantics, hook CPU evidence and readiness cancellation corrections.

Cross-compilation linked kernel, bridge and client. Forced disabled kernel build
verified zero irq_drain exports. No new host simulation, native VM execution or
physical test. Build attempts preserved: stale disabled object link failure,
network-disabled missing runtime download, then empty extraction directory.
Forced object rebuild and successful package fetch/re-extraction resolved these.
Existing CPU-routing array-bounds warning remains present in both configurations.

Next: matching artifacts into fresh disposable four-vCPU VM, verify serial/hash
identity, enforce host watchdog, execute and inspect all eleven round results.
Close/team-exit runtime controls remain separate, as do failed-removal driver
containment, semaphore owner exit and physical memory/recovery validation.
Evidence: testing/kernel-irq-fixture-build-001; no physical acceleration claim.

## Astra: native IRQ drain checkpoint passed

Two four-vCPU VM boots passed2 sequential controls and20 overlap rounds total.
Actual kernel dispatcher/removal, vector24, CPUs0/1/hook1, callback1, no timeout;
all overlap event orders1,2,3,4 verified independently by host parsers. Exact
serial revision and kernel/package/bridge/client hashes checked before execution.
Repeat shutdown corroborated by QMP SHUTDOWN guest:true/reason:guest-shutdown.
First-run shutdown claim limited to process exit0 after request; raw result kept.

Implemented this checkpoint: reproducible isolated VM runners, independent log
verification, corrected cached revision stamp/provenance. Compiled: stamped kernel
and bridge rebuilt; client from reviewed cross-build. Host simulation: no new run.
Physical: none; no Intel GPU command execution or acceleration demonstrated.
Initial wrong-stamp boot rejected before fixture; logs retained at kernel-irq-001.
Evidence and limitations: tests/evidence/kernel-irq-native-002/ and
workstreams/gpu-next/native-irq-vm-validation.txt. Terra independently accepted
first-run numeric/identity evidence; repeat evidence review requested.
Next: close/team-exit controls and remaining production IRQ/semaphore lifetime
fixes; controlled physical memory/MMIO/display/recovery validation still required.

## Astra: native semaphore owner-lifetime behavior reproduced

Reviewed C++ userspace test confirms blocked waiter then owner exit/exec. Both
cases passed on recorded baseline kernel: waiter and old-ID info/count/release/
delete all B_BAD_SEM_ID; replacement IDs distinct only, no same-slot/wrap proof.
Native -Wall -Wextra -Werror compile passed; QMP guest-shutdown corroborated.
No new host simulation, candidate kernel/driver install, physical test or GPU work.

Accepted source distinction: public retrace notification can disappear while
other clients survive; full-ID checks provide immediate rejection, not evidence
of a semaphore-pointer UAF. No ABI-breaking ownership change made. First GPU
completion remains independent retained-worker/status-polling design. Driver
notification defect, callback/lifetime/physical gates remain open. Physical
192.168.1.114:22 probe timed out; current access/recovery details requested.
Evidence tests/evidence/sem-owner-001/; research/n100-submission/RETRACE-LIFETIME.txt.

## Astra: native close overlap validated

Implemented reviewed close mode in isolated source2d71916f; kernel/bridge/client
cross-build passed. One native close case passed strict ENTER/CLOSE/EXIT1,2,3,
correct CPUs, count1, no timeout/duplicates, retained exact handler/vector,
reopening denied, bridge tail before Free. Uninit observed afterward. Terra
independently reviewed evidence; descriptor source trace supplies return lifetime.
Fresh-boot normal11-round regression also passed; both QMP guest-shutdown verified.
No new host simulation, physical operation or GPU execution. Production GPU tree
unchanged. Tests/evidence/kernel-irq-close-001 records scope and hashes.
Remaining: team-death behavior, production teardown containment, physical memory/
MMIO/display/recovery validation. User access/recovery question still pending.

IRQ removal review checkpoint: normal semaphore cleanup withheld. Actual kernel
dispatch/removal host extraction reproduces no-lock node UAF under ASan; locked
control completes. Terra independently reproduced. See IRQ-TEARDOWN-CONTRACT.txt
and tests/evidence/irq-remove-contract-002. No new native/physical testing and no
production edits; physical GPU execution remains unproven.

Isolated kernel5ccc8871 protects already-locked dispatch accounting through
vector retirement. Host matrix4baseline failures/4candidate passes; normal and
instrumented kernels compiled,11native VM regression rounds passed. See
tests/evidence/kernel-irq-tail-001. No physical test or production integration.

PHYSICAL MEMORY001: native089 built on N100, installed on RenkuTest USB and
rebooted. Matched loaded components verified;30owned allocate/free cycles passed,
HDMI and app_server preserved. Evidence tests/evidence/physical-memory-001.
This is actual physical memory smoke, not GPU command execution or complete
memory-safety validation. Independent review and repeated boot pending.

A26: native owner/fixture build and302/10 mock cases passed. Hash-checked old
owner backup and candidate installed on RenkuTest; reboot and six archived live
PCI observation sessions passed.10 paired GART rounds/20 alloc-free also passed;
HDMI/app_server preserved. Terra independently accepted source and physical
evidence. Helper905b808, incremental owner patch0089, evidence physical-pci-owner001.
No GT MMIO/commands/acceleration; next mapping/order and ACK inventory gate.

A27: explicit UC same owner BAR plus v2 pinned fixed GT ACK observation.
Native owner/fixture build and26 mocked cases pass;host ASan/UBSan26 independently
pass. Fresh reboot and eight physical130044 raw0 samples pass;20 paired GART
alloc/free and pre/post PCI tests pass. HDMI/app_server preserved, display/GART
hashes unchanged. Terra accepted source/native/physical evidence. Incremental
patch0090 and evidence physical-gt-ack001. No GT stores/handshake/GPU execution.
Next A28 bounded forcewake authority/power/order/uncertain-cleanup design.

A28: native primary forcewake state/owner/fixture compiled;67+37 host/native
mocked cases pass independently.21 artifact checks,A27backup andfreshboot.
ONE physical client:ACK0->1->0,9reads,2writes,454us,Complete/Released,exit0.
20paired GARTalloc-free,read-onlyACK/PCI andHDMI/app_serverregression pass.
Terra accepted source/native/physical evidence.9bb30b3 pure,cc5befd owner,
patch0091/evidencephysical-forcewake001. NoGPUcommands/DMA/acceleration.
Next A29engineinventory pluscontext/reset/memory/DMA/completion safety.

A29offline candidatefa6102a/96fd967: heldfixedBCSsampler,84/37/48hostchecks pass
independently. Native/physicalNOTRUN,transferblocked bytargetSSH/HIDstalls.
TransientSSHrecoveryallowedrebootattemptbutaccessnotrestored; Danresetrequested.
NoA29deployment orGPUexecution. Nextnativebuildafterdesktop/HIDrecovery.

## GitHub publication — 2026-11-07

User requested all progress on GitHub. Published complete milestone history to
private chevydan6/haiku-intel-gpu, gpu/native-memory through f91feaa; verified
remote SHA. Added missing recovery patches 0069–0079, matching host harnesses,
GART caller and IRQ defect records, all shared coordination files, README and
results/2026-11-07-gpu-progress.md. Issue #1 tracks execution gates. All 92
patches reconstruct and existing host CI checks pass; Terra reviewed the report.
No new native compilation, VM run or physical GPU test. A28 physical handshake
and A29 host-only candidate remain distinct; GPU command execution incomplete.
Private default branch fast-forward follows; RenkuOS Source main is untouched.
