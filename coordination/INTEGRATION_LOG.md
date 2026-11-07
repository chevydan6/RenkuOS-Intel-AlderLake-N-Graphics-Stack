# Integration log

2026-11-07: preserved production diff in snapshots/. Existing Terra worktree discovered at /home/dan/renkuos-terra-n100; newly created source/terra-review-worktree is an unused review snapshot, not a second active agent. No duplicate Terra launched.

Terra R1 accepted: callbacks nested sLock through get_aperture. Removed helper acquisition, documented caller-owned lifetime lock. Threaded fixture now includes actual production helper and passes 100 rounds. Native revalidation and Terra verification pending. Previous native builds compiled the code but did not expose this actual helper locking defect. No physical deployment.

Build 073 native actual-lock regression and finalizer passed; no physical deployment. R2–R5 accepted for correction/design. R1 remains awaiting Terra independent verification.

## Terra independent host revalidation (2026-11-07)

R1 corrected helper/header/query passes isolated host control; actual-helper
query/unmap fixture passes 100 ASAN/UBSAN rounds. Old snapshot is rejected by
bounded negative control. No Terra production patch or merge; no physical test.
Research and review delivered in /home/dan/renkuos-terra-n100. R2–R5 await Astra
assessment; T-F1 direct-ring proposal and T-F3 module findings await design review.

Astra R2 patch 0076: native build 074 passes bundled tests, 18 recovery/publication cases, 2 actual-lock queries and artifact finalizer. VM clean shutdown. CreateMemory publication fault fixture is host-only. Evidence installation/private/gpu-hdmi-build-051/. Awaiting Terra validation; R3–R5 remain open.

Build 075 passes native compilation, bundled tests, 18 recovery cases, 2 real-semaphore query cases and host finalizer. VM clean shutdown. Patch 0077 aperture semaphore cleanup covered by 200 host-only destructor cases; previous source fails negative control. Manager-level R3 and R4 remain open; independent Terra validation pending. No physical hardware test. Evidence installation/private/gpu-hdmi-build-052/.

Terra R2 source/host follow-up: actual initial hash-bucket allocation failure test
passes 100 faults + retry; old source negative control fails; independent recovery
fixture run passes 18 cases. Delivery patch dry-run passed. No production merge,
commit, native/VM operation, hardware deployment or GPU execution by Terra.


Build 076 passes native compilation, bundled tests, 18 recovery cases, 2 real-lock queries and artifact finalizer. VM clean shutdown. Expanded real-hash teardown regression remains host-only. R4 independent validation pending; manager-level R3 and borrowed backing R5 remain open. Evidence installation/private/gpu-hdmi-build-053/. No physical test or GPU execution.

### Astra R3 manager lifetime correction, patch 0079

agp_init drops PCI reference on lock creation failure before constructing the
registry. Successful agp_uninit releases hash storage, reconstructs an empty valid
global table, destroys manager lock and drops PCI. Failed DeleteAllMemory returns
before those releases, preserving retry ownership. Extended existing
gart-uninit-host.py rather than keeping a duplicate harness. Production init/uninit
and real hash headers pass initial-lock-failure, 256-record successful teardown,
partial cleanup failure/retry and reinitialization checks under ASAN/UBSAN.
Old build 076 source fails runtime init-unwind assertion. OS semaphore/PCI/backend
calls are mocked; no real kernel resource exhaustion or unload stress is claimed.
Build 077 passes native compilation, bundled tests, 18 recovery cases, 2 real-lock
queries and finalizer; VM exits cleanly. Evidence installation/private/gpu-hdmi-build-054/.
Terra: please independently review R3 completion and global-object reset ordering.
No physical test. R5 external-backing ownership and independent review remain
required before physical memory deployment or submission.

cd65120 integrates reviewed Terra reports, pinned source archive and ownership proposal. Source/archive checks and independent R1/R2 host scripts pass. No new driver compilation required for this documentation/research integration; latest native remains 077. No physical tests.

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

## IRQ removal contract characterization
Added tests/irq-remove-contract-host.py; GCC13.3 ASan host compilation successful.
Evidence irq-remove-contract-002: locked control exit0; no-lock exit1 with expected
heap-use-after-free and callback still active at removal return. Terra independently
reproduced initial schedule. No production integration, native build/VM test or
physical deployment in this checkpoint. Cleanup design withheld pending lifetime
contract; first GPU workload remains gated.

A11 isolated kernel commit 5ccc88710073fe2ee2e5691e69434869cda6b46b: retain vector lock through accounting.
Four host baseline failures/four candidate passes; both kernel builds compiled;
11 native VM regression rounds passed with QMP guest shutdown. Production GPU
worktree unchanged. No-lock expected defect persists; physical untested.

Physical native089 components installed on expendable RenkuTest USB; original
components retained in /boot/home/gpu-memory-089-001/recovery and host baseline
images preserved. One physical30-cycle owned-memory test passed. Independent
evidence review pending; hardware GPU execution still unproven.

Physical-memory002 repeats30serial cycles after warm reboot via visible NanoKVM
terminal. No component changes. Evidence tests/evidence/physical-memory-002.

CONCURRENT DIAGNOSTIC — 2026-11-07
Implemented physical/memory-concurrent.cpp: two clients, both live ranges checked
before either free, explicit retention on ambiguity and worker lifetime tracking.
Terra reviewed; cancellation diagnostics improved. Source SHA256:
8c27c133808537f2efef15928035000e0f5e70895c3cc81e4ffcba5b22623d09
Host compilation/simulation: Terra harness passed success, overlapping ranges,
allocation failure and thread-creation failure; mocks replace OS/driver calls and
quarantine becomes exit77. This does not test a stuck kernel ioctl or real GPU.
Native compilation: NOT performed for this diagnostic. File transfer timed out;
NanoKVM typing failed with /dev/hidg0 I/O timeout before source entry.
Physical: no new test. Prior60allocation/free cycles across2boots remain evidence.
NanoKVM reachable, USB configured; target responsiveness unknown. Dan is checking
laptop. No reboot attempted. Hardware operations paused pending that check.
Remaining: native compile/range controls, observed concurrent-client test, broader
lifetime prerequisites, then GPU submission. GPU execution remains unproven.
Host evidence: /home/dan/renkuos-terra-vulkan/test-controls/concurrent-host/result.json

PHYSICAL CONCURRENT CHECKPOINT — 2026-11-07
Implemented: reviewed two-client diagnostic with pre-free range checks and retention.
Compiled on physical N100: C++17 -Wall -Wextra -Werror -pthread; exit0.
Native no-device checks:8 range controls passed. Host simulation:4 Terra mock cases
passed; not real driver execution. VM: none new.
Physical:10 paired rounds,20allocations/frees passed exit0. Both live returned ranges
checked before release; app_server127 before/after, visible1920x1080 terminal intact.
Six installed component hashes match prior native089; USB RenkuTest and PCI46d1.
Source8c27c133808537f2efef15928035000e0f5e70895c3cc81e4ffcba5b22623d09.
This is not GPU command execution or proof of simultaneous kernel execution.
User reset resolved preceding terminal/HID unresponsiveness; cause unresolved.
Boot log includes xHCI endpoint stall. No claim of error-free system or full lifetime
safety. Remaining: independent physical evidence review, lifetime/retirement gaps,
engine/context/VM/cache/reset requirements before first GPU workload.
Evidence: gpu-driver/tests/evidence/physical-concurrent-001/

IDLE-HANG INVESTIGATION — 2026-11-07
Implemented: no production changes; submission design updated with physical memory
milestones and user-authorized recovery scope. No new compilation or simulation.
Physical: screensaver preferences showed Enable screensaver unchecked and screen
power-off disabled. No power setting changed. The hexdump command was unavailable;
GUI screenshot is evidence, not a parsed settings-file result.
Six10-second heartbeat intervals completed in visible NanoKVM terminal; app_server
team127 remained running. This is a lightly active responsiveness observation,
not true idle/suspend testing and not proof against longer idle failures.
Saved syslog deep-sleep messages belong to mediatekwifi; xHCI stall/recovery is also
present. Neither establishes a system suspend. Prior terminal/HID hang remains
unexplained. No new GPU command workload or acceleration evidence.
Independent latest physical-evidence review remains pending: Terra task returned
service cybersecurity flag/systemError. Earlier diagnostic source/mock review
was completed; do not conflate that with physical evidence review.
Next: resolve review availability and remaining ownership/retirement/submission
prerequisites. Do not assume this observation satisfies GPU execution gates.

PAIRED OWNER LIFETIME TESTS — 2026-11-07
Implemented: deterministic two-operation retirement coverage in existing owner
state-machine suite. Both leases revoked with pins outstanding; both return orders
and all four success/failure combinations repeated100times (800cases). Verify
request slot cannot be reused, stale leases rejected, first return cannot release
backing, and any failed return preserves retention after final pin drops.
Compiled: host and actual N100 C++17 with warnings as errors; both passed existing
100close races plus800new cases. Source/header SHA256 matched target copies.
Simulation: userspace state-machine harness uses actual header with modeled backend
return events, not kernel GART calls. Two host-only deliberately broken copies
failed at expected assertions; unmodified control passed. Native test visibly
run through NanoKVM; no new physical GPU ioctl workload or GPU execution.
Production owner/display code unchanged: these tests found no new defect.
Incomplete: backend/driver failure and close/unload integration, interrupt lifetime,
engine/context/VM/cache/reset/submission. Independent review pending; Terra service
review unavailable in latest attempt. Previous physical memory evidence unchanged.
Evidence: tests/evidence/owner-paired-001/

ACTUAL OWNER ADAPTER COVERAGE — 2026-11-07
Implemented tests extracting unchanged Status, Operation, Query, Allocate, Free,
FinishDisplay and ReleaseDisplay functions from IntelMemoryOwner.cpp. Controlled
GART calls gate return while another thread releases the display lease; tests
check status/argument forwarding, stale-call rejection, active pins and failed
backing retention during initialization and normal ready state (12cases).
Compiled/tested: host C++17 and native N100 via visible NanoKVM; both12cases pass.
Native initial build failed on status-constant macro collision; harness corrected
and rebuilt successfully. Production adapter unchanged. Input hashes verified.
Simulation: GART and kernel mutex modeled; success/failure injected. Two generated
host adapter regressions failed expected assertions. No real backend allocation,
MMIO, module unload, GPU ioctl or GPU command execution in these new tests.
Physical: N100 ran the userspace harness only; previous physical allocation/free
evidence unchanged. No new VM test. No hardware acceleration demonstrated.
Incomplete: BeginDisplay/resource acquisition and module lifecycle are excluded;
real GART failure/close integration, interrupts, execution memory/cache/context/reset
remain. Independent review pending; no production integration claimed.
Evidence: tests/evidence/owner-adapter-001/

PHYSICAL ALLOCATION COPY-OUT FAILURE — 2026-11-07
Implemented memory-copyout.cpp: one readable but nonwritable request/result area,
then one normal allocation/free. The matched driver source reads the request,
allocates via retained owner/GART, and on result-copy failure calls normal free.
Compiled natively on N100 with warnings as errors; help-only and physical run
completed. No new simulation or VM test. Real physical ioctl returned-1 with
errno B_BAD_ADDRESS(-2147478783); protected request unchanged; subsequent normal
allocation/free passed; overall exit0. app_server127 stayed alive and visible
NanoKVM terminal returned. Four loaded memory/driver hashes match prior native089.
No copyout rollback failure message found in collected syslog. This is consistent
with the reviewed rollback path, not independently instrumented branch/resource
accounting: a backend returning the same error is not separately distinguished.
No guessed free of hidden buffer, no borrowed mappings, no GPU commands or writes
to display buffers. This is not leak freedom, actual client-close overlap, engine
execution, or phase1 completion. Kernel fixture remains unchanged/uninstalled.
Next: independent review and remaining real close/retirement checks; GPU memory,
context, cache and reset contracts still gate workload submission.
Evidence: tests/evidence/physical-copyout-001/

OFFLINE PRIVATE GPU PAGE-TABLE ENCODER — 2026-11-07
Implemented isolated IntelN100Ppgtt.h: checked4KiB system-memory leaf/directory
encoding, raw PAT selector,39-bit DMA limit,48-bit GPU ranges and four table
indices. Outputs unchanged on rejection. No production/display integration.
Terra independently researched pinned i915 and accepted design before coding;
review caught unfixed enum invalid-value conversion, corrected to uint8_t enum.
Terra independently reran corrected2150cases with UBSan and accepted offline
milestone. Source references/design in research/n100-submission/PPGTT-ENCODING.txt.
Compiled: host C++17 warnings-as-errors+UBSan2150pass; nativeN100 C++17 warnings-
as-errors2150pass, build/test exit0, exact source/header hashes matched. Build
and execution visible viaNanoKVM. Physical machine ran CPU-only offline tests.
Simulation: no GPU simulator; tests check pure encoding/arithmetic. No installed
GPU mappings, real DMA, GPU workload or hardware-acceleration proof. No new VM test.
Remaining before hardware publication: owned pinned page backing/DMA translation,
scratch, actual PAT/cache setup, barriers/TLB invalidation, permissions, context,
completion/retirement/reset, and outstanding lifetime safety gates. General memory
close/unload and independent prior physical-evidence review remain incomplete.
Next: owned private page-table construction using reviewed encoding, still offline
until resource lifetime and hardware publication contract are validated.

OWNED OFFLINE PPGTT CONSTRUCTION — 2026-11-07
Implemented IntelN100PpgttTree.h: fixed ownership ledger for4active table pages,
3scratch table pages,1private scratch data page and1..16owned diagnostic pages.
One2MiB-aligned raw48-bit GPU window. Full scratch-backed tree; all CPU/DMA/token
validation precedes page writes. Failure rolls back known allocations; failed
release retains ledger/context for explicit retry. Ambiguous descriptors retain
all claims with no guessed cleanup. No destructor cleanup or publication API.
Backend and record lifetime, external serialization and callback non-reentry
are explicit requirements; backend must guarantee exclusive physical backing.
PAT selectors restricted0..3 per pinned N100 platform policy. Scratch aliases
are writable present mappings, not faulting guards or verified isolation.
Compiled/tested: host C++17 warnings-as-errors+UBSan69cases passed; independent
Terra run also69passed. Actual N100 native build/test exit0, matching input hashes,
visible NanoKVM. Full independent entry/walk checks; all24acquisition failures,
all24release failures/retries,rollback failure,8ambiguity variants and admission.
Simulation: allocator/DMA descriptors are modeled. Native execution allocates
ordinary userspace test RAM; it does not install real GPU tables or access GPU.
No new GPU ioctl, VM run, physical execution or acceleration result. Existing
production driver/display remains unchanged; this is an isolated candidate.
Terra accepted design before implementation and final corrected code after
independent tests. Coverage additions included uncached directories,1data page,
distinct PATs,invalid cache and48-bit boundary; no blocking review defect remains
within offline scope. Hardware readiness is not approved.
Next: real retained kernel-page allocator and DMA translation adapter, then
cache/PAT/TLB publication contract. Context, completion/reset and existing
lifetime gates remain required before any GPU workload.
Evidence: tests/evidence/ppgtt-tree-001/

KERNEL PAGE BACKING ADAPTER — 2026-11-07
Implemented IntelN100KernelPages.h: exclusive wired4K kernel areas, monotonic
private ownership tokens, physical-entry coverage validation, fixed24slot ledger,
context admission and conservative retention. Mapping validation failures return
acquired descriptors for tree quarantine; no hidden failed-allocation cleanup.
Unexpected delete errors stay sticky until reboot to avoid recycled area-ID retry.
Provider requires resident module/code and externally serialized records.
Compiled: N100 native kernel flags/headers and linked against real kernel exports
with --no-undefined, exit0; VM API imports inspected. Artifact has no module entry
point and was not installed/invoked. Build/test actions visible through NanoKVM.
Initial optimized compilation exposed potential uninitialized scratch encoder
output; explicit success guards added in tree, then compile/link passed.
Host O2+UBSan:35mocked VM provider cases and69tree cases passed. Terra independent
review/run accepted provider; tree success-guard follow-up review tracked separately.
Mock fixture persisted; failure cases include invalid CPU(no map),late count error,
create failure with rollback-delete failure,held-context admission and sticky delete.
Physical hardware runtime: no new allocation or GPU execution in this checkpoint;
physical laptop performed compilation only. No new VM test. CPU physical lookup
is not a verified GPU DMA/IOMMU mapping. Production driver remains unchanged.
Next: separately reviewed resident kernel fixture to invoke real page allocation,
verify owned tree/cleanup and record native runtime. Hardware publication remains
gated by DMA/cache/PAT/TLB,retirement/context/reset and unresolved lifetime checks.

RESIDENT KERNEL WIRED-PAGE FIXTURE — 2026-11-07
Implemented tests/ppgtt-native-{module,bridge,client,protocol} and make targets.
B_KEEP_LOADED module owns provider/tree/lock and cached one-shot result. Three
cases create1,16,1diagnostic pages at low/top windows; verify all table entries,
independent walks, zero data/scratch, kernel area4K/full-lock attributes, release
and empty ownership ledger. Old-ID absence is supporting observation only.
Bridge owns no retained resources; copyout failure cannot rerun/clean module.
Terra reviewed design and source, accepted bounded memory-only physical test.
Compiled on actualN100 through visible NanoKVM: module,bridge and client pass,
source and binary hashes saved. No new simulation/VM test in this checkpoint.
Runtime: NOT installed or executed. NanoKVM HID timed out before installation.
SSH remained responsive; attempted visible Terminal launch stalled before its
logfile appeared; launch shell2634 was subsequently sent termination. One normal
shutdown-r request returned0 but uptime remained1hour; reboot completion unproven.
Thus console/target recovery blocks real kernel allocation evidence. Physical
reset needed; test fixtures did not cause this pre-install hang. No inference of
sleep cause, error-free system, actual page reclamation, DMA or GPU execution.
Next: recover console, verify fresh boot/identities/artifacts, install only named
test module/bridge, observe client within60sec under canonical locks, collect logs
and leave resident failure state. Existing driver/recovery components untouched.

FIRST NONCE PAYLOAD ENCODING — 2026-11-07
Implemented checked8DWORD GGTT nonce/complement stores at dedicated4K page
slots0/4, below4GiB. Raw addresses are GGTT offsets, never CPU/physical pointers.
Page alignment/range, output capacity and unchanged invalid outputs are checked.
Nonce poison/randomness policy is separate; encoder preserves all32bit values.
Compiled/tested host O2 C++17 -Wall -Wextra -Werror UBSan:32cases passed. Literal
reference packet, edge pages, complement, guard words and rejection nonmutation.
Terra independently reviewed source and reproduced32passing cases; accepted
as offline candidate. No native compilation, GPU simulator, VM or physical test.
This is only a direct-ring payload, not a complete request. It has no flush,
completion, context, ring control, MMIO, publication or recovery operation.
Existing production display/source unchanged. GPU execution remains unproven.
Physical blocker revalidated: NanoKVM tools/list has only HID/video controls,
no reset/power. Console recovery still requires physical reset; no new module
installed/run. Kernel fixture14ba4f8 remains priority after recovery. More offline
packet checks do not substitute for its real-kernel evidence.
Next: run reviewed kernel page fixture once console recovers; complete exact
Gen12 visibility/flush/completion and initialization/reset contracts before GPU
publication. Desktop status updated and goal remains active.

BRING-UP ADDRESS/POWER AUDIT — 2026-11-07
Implemented documentation of exact GT forcewake handshake/domain and a source-
verified DMA-address contract. Terra independently reviewed both; stricter MMIO
error handling and2.5second aggregate deadline are proposed adapter policy, not
Linux equivalence or a validated physical response limit. The x86 PCI ram_address
identity helper is for resource conversion, not a per-device DMA mapping API.
Archived log has DMAR presence; applicable units/scopes and platform constraints
remain unverified. Inactive remapping alone would not establish full DMA readiness.
Compiled: none new. Simulation: none new. Physical: SSH recheck timed out; cached-
looking NanoKVM terminal unchanged, no input/reset capability recovered. No new
kernel fixture installation/run, MMIO writes, GPU workload or acceleration.
Next: physical reset enables the reviewed wired-page fixture; then live DMA and
GT ownership/power/forcewake checks before engine initialization/submission.

HARDWARE BLOCKER REVALIDATED — 2026-11-07
SSH banner exchange times out; NanoKVM mouse HID write returns I/O timeout.
Same console/target condition has persisted across at least three goal turns.
Normal reboot did not restore access; available NanoKVM MCP has no power/reset
control. Required real-kernel fixture remains compiled/reviewed, uninstalled and
unexecuted. Next milestone cannot proceed without physical laptop reset.
No implementation, compilation, simulation or physical GPU test this checkpoint.
Goal marked blocked, not complete. Resume after user reset, verify fresh uptime,
PCI/artifact identities and visible terminal before running the reviewed fixture.
GPU command execution and hardware acceleration remain unproven.

REAL KERNEL PAGE/TREE RUNTIME — 2026-11-07
User physical reset restored console. Fresh uptime and USB RenkuTest boot,PCI46d1,
kernel hrev60072+518,loaded display path verified. Old USB hash log was empty and
existing binaries differed after reset; re-copied all reviewed sources and rebuilt
natively. Exact source hashes match; rebuilt binaries reproduce original reviewed
hashes. Checksum verification and byte comparison preceded test-only installation.
Implemented this checkpoint: evidence collector/status; fixture source unchanged.
Compiled actualN100 module,bridge,client passed. Ran actual resident B_KEEP_LOADED
module through visible NanoKVM terminal. Three kernel cases passed:1/16/1owned
diagnostic pages,42wired areas verified,full table/independent walks and zero data,
all releases succeeded,empty provider ledger/state,42old IDs absent (supporting
observation),cached second ioctl identical,client exit0. Module remained loaded
after client closed; app_server127 unchanged and display/terminal responsive.
No new simulation or VM run; this is real kernel CPU-memory work on physical
hardware, not GPU execution or GPU DMA/cache/translation proof. No GT MMIO writes.
Production driver/accelerant/GART hashes unchanged. Kernel logs still contain
xHCI endpoint configuration/stall errors; system-wide stability not established.
Only test module/bridge installed; resident test state remains until reboot.
Terra independent physical evidence review requested. Remaining:full failure/close
lifetime gates,device DMA/IOMMU/PAT/visibility,TBL/context/power/reset/submission.
Next: accept independent runtime review and implement bounded GT/DMA bring-up.

READ-ONLY PHYSICAL DMAR INVENTORY — 2026-11-07
Implemented bounded pure DMAR parser plus cached resident snapshot module,
bridge/client. Source-specific ACPICA lifetime reviewed:public get_table acquires
reference with no matching publicput. This diagnostic retains one table reference
and ACPI dependency forboot lifetime; repeatedcalls never reacquire. Native kernel
paths show no normal runtimeDMARuninstall while retained module is active; this
is not a universal validation-ref guarantee or arbitrary-module exclusion proof.
Native N100 build/module/client passed;96parser cases passed hostUBSan andnative.
Terra independently reviewed and passed96hostASan/UBSan cases. No new VM test.
Physical read succeeded,version1,status0,cached1,exit0;136byte valid-checksum table:
DRHD0 registers0xfed90000,segment0,endpoint00:02.0 (GPU8086:46d1 rev00).
DRHD1 registers0xfed91000,segment0,include-all,IOAPIC00:1e.7/HPET00:1e.6.
Raw recordRMRR,segment0,inclusive0x7c000000..0x803fffff,endpoint00:02.0.
Parser preserves RMRR as other_record; raw evidence independently decoded. No
claim that firmware-specified coverage means translation enabled or disabled.
Unknown records/flags/scopes remain unresolved; source-specific residency and
bounded4KiB/16unit capture limits are explicit. Missing/oversize/malformed leaves
DMAstatusunknown. No controller/PCI/MMIO register reads or writes, GPU commands,
or DMA identity proof. app_server127 unchanged; console remains responsive.
Final source/binary andinstalled hashes checked. Earlier native build appears in
screenshot but superseded by final rebuild. Terra independently verified rawbytes,
source/build/installed identities,result,PCIendpoint andbefore/afterapp_server.
Module and ACPI/table refs remain retained untilreboot. Production display/driver
unchanged. Next: reviewed read-only remapping-register state andreserved-memory
exclusion policy,then GTownership/power/forcewake before command submission.

READ-ONLY PHYSICAL VT-d STATUS — 2026-11-07
Implemented a separate resident one-shot observer depending on the cached DMAR
snapshot. Validates all DRHD bases before mapping; requests explicit UC and kernel
read-only4K mappings; narrows reads toGSTS32,VER32,CAP32low/high,ECAP32low/high,
VER32,GSTS32. No controller stores. Cleanup errors retain mapping records/module.
Module/client copies expose raw samples and conservative Unknown/Disabled/Enabled
classification. Cached subsequent calls never repeat hardware accesses.
Reviewed assumptions: trusted firmware DMAR; advertised pages are in a non-RAM
gap in captured EFI map,not explicitly MMIO descriptors. No competing IOMMUmanager
found. Kernel typed-alias conflict can returnB_BUSY orpanicdebugkernel; normal
sandbox recovery accepted. ExplicitUC avoids weak type adoption. MMIO CPUfault/
hang cannot be caught as callbackfailure; external observer/recovery bounds trial.
Compiled actualN100 with kernel flags and --no-undefined passed;source/installed
binary hashes match.79hostUBSan/mock read cases and79native mock cases passed.
Terra independently ASan/UBSan-tested actualmodule in3mapping/cleanup/cache mock
scenarios, source-reviewed and independently verified physical evidence.
Physical results (all map/read/cleanup0;client0;cached1):
unit0 fed90000 VER40/40 CAP01c0000c40660462 ECAP29a00f0505e GSTS0/0;
unit1 fed91000 VER50/50 CAPd2008c40660462 ECAPf050da GSTS0/0.
TES31 clear at four sampled times. This does not prove no change between/after
samples,identity DMA,translation ownership,RMRR handling orGPUaccess. VM mapping
metadata/type accounting changes are distinct from controller-register stores.
app_server127 unchanged;visibleNanoKVM terminal responsive. Module/dependencies
remain resident untilreboot. Production GPUdriver/display components unchanged.
No GPUcommands or hardwareacceleration;no newVMtest. Next:reserved-memory exclusion
and explicitdevice DMAownership,then GTpower/forcewake/context/reset/submission.

REQUIRED FIRMWARE RESERVED-RANGE FILTER — 2026-11-07
Implemented bounded RMRR extraction and inclusive4K range exclusion. Parser rejects
malformed/reserved/unknown relevant structures,scope errors and>16ranges; includes
all RMRRs regardlesssegment/scope conservatively,not a PCI topology resolver.
Kernel page provider now requires valid copied configuration before acquisition.
Configuration commits atomically and cannot change while active/live/retained.
After physical lookup and before tree page writes, collision leaves descriptor
invalid and all acquired areas quarantined untilreboot; no guessed release. This
availability limitation is intentional. Kernel VM's own page initialization is
not excluded by this later check; it does not replace firmware reservation handling
inside VM. No GPU publication is introduced.
Host O2UBSan:36filter boundary/structure cases and39provider/mock VM cases passed;
Terra independently reproduced withASan/UBSan. Early/late collision no-tree-writes
and no cleanup/admission,unconfigured rejection,failedreconfigure andactive rejection
are modeled tests. Native kernel build/client and36purefilter cases passed.
Updated resident fixture version2 uses distinctmodule/node (originalv0 untouched),
retains cachedDMARdependency,configures actualsnapshot and independently compares
all allocated physical4K spans against rawRMRR records rather than policy helper.
PhysicalN100 result:version2,status0,3cases,42areas verified,42exclusion checks,
1range,allrelease calls succeeded,providerlive0/treeEmpty,cachedidentical,exit0.
app_server127 unchanged;visibleNanoKVM responsive. Current firmware range is
0x7c000000..0x803fffff inclusive from priorread-onlyDMARsnapshot. Syntheticcollision
retention remains host evidence;positive physical run validates allocatedpages only.
No newVMtest,GT MMIO orGPUcommands. This does not establish deviceDMAownership,
IOMMU lifetime,cache/PAT/TLB,GTpower/forcewake,context/reset/retirement oracceleration.
Terra source/oracle review accepted before physical; evidence review requested.
Next:GTpower/forcewake owner and bounded handshake;keep publication gated.

A26 integrated into isolated RenkuTest USB: checked native owner candidate
3ca11353965ec0100795c2896ce14ab9050cc0204a0fd2c7af4d06080d78d2bd
replaced previous owner only after19 source/artifact/baseline/backup checks.
Old a0e7ed... backup preserved on USB and host. Software reboot succeeded and
NanoKVM Terminal displayed live PCI checks. Native302+10 mocked tests passed;
host old12 adapter cases and2 controls passed. Six archived real PCI sessions
and10 paired GART rounds/20 alloc-free passed; app_server127 after boot through
final, HDMI working, old display/GART binaries unchanged. Terra accepted evidence.
Owner pin is not global PM; no GT MMIO/DMA/commands. Helper commit905b808; owner
patch0089 and physical evidence recorded as separate milestones.

A26 recorded commits:905b808 pure helper,e5b919d owner/fixture,41b0139 physical
evidence. Patch replay against preserved before snapshots matched all three
production outputs byte-for-byte. No unrelated uncommitted work was staged.

A27: explicit UC on same boot-time owner mapping and narrow v2 observation
integrated into RenkuTest after19 source/artifact/baseline checks. A26 backup
verified USB/host. Native build/link and26 mocks pass. Fresh software reboot
through NanoKVM reached HDMI desktop; fixed130044 observed eight rawzero values
across live/post-memory calls.10 paired GART rounds20 alloc/free and pre/post
PCI sessions passed;app_server127 retained after boot;old display/GART hashes
unchanged. Terra independently verified. No GT stores/commands/acceleration.

A27 tested commits:933aa65 implementation/design/tests;ffe905a physical
evidence. Incremental replay matched source; previous uncommitted work preserved.

A28 integrated reviewed v3 into RenkuTest after21 checksum gates and A27owner
backup29570eb... onUSB/host. Native owner e195cd...,bridge41b314...,client4aa4b6...
linked with --no-undefined;67 state+37 owner mocks pass. One physical NanoKVM
client invocation succeeded:ACK0->1->0,9reads,2writes,454us,Released/Complete.
No retries.20 paired alloc/free and read-onlyACK/PCI regressions pass. HDMI and
postboot app_server127 preserved;existing display/GART binaries unchanged.
Terra independently accepted. Commits9bb30b3 pure,cc5befd owner;physical evidence
separate. No fallback/engine/DMA/GPUcommands.

A29 offline checkpointfa6102a/96fd967:84 state/37sampler/48actualowner host
ASan/UBSan cases pass with independentreview. NativefirstsourceSCP timedout
twice beforecompilation; candidateNOTinstalled. NanoKVMkeyboardHIDg0 I/Otimeout
andlatermouseHIDg1timeout. Read-onlySSHbriefly returneddate/uptime24min,then
controlledrebootrequest timedout andvideostayedlastA28prompt; no recoveryconfirmed.
Physicalreset requested,USBretained. No causalclaim sleep/drivercrash andnoA29
hardware inference. PreserveA28knownartifactse195cd.../recovery.


## GitHub publication preparation — 2026-11-07

Archived previously untracked recovery patches 0069–0079, matching host harnesses,
GART caller audit, IRQ defect evidence and coordination records with the later
milestone series. All 92 patches reconstruct from pinned source. Host source and
payload audit, EDID, GGTT, GART range/lifecycle/allocation, kernel resource,
boot-framebuffer, offline packet and Python syntax checks passed. Current
lifecycle fixture: 154 checks; original control: 39 expected failures.
No new native compilation, VM test or physical GPU test occurred during this
publication. A28 physical handshake and host-only A29 remain separate milestones.
New README/current report identify incomplete execution gates and target reset
blocker. Downloaded Linux source caches and private access/recovery files remain
local. No RenkuOS Source main integration is performed here.

## GitHub history publication verified — 2026-11-07

Published the complete local milestone history through f91feaa to
https://github.com/chevydan6/haiku-intel-gpu/tree/gpu/native-memory and verified
its remote SHA as f91feaa92172883b19322461eed110f102f24007. The former default
branch head 81f11d6 is an ancestor; no remote commits are discarded. Tracking
issue: https://github.com/chevydan6/haiku-intel-gpu/issues/1 . Terra independently
reviewed the current report; raw MSG_IDLE_BCS naming and reviewed-log wording
were corrected. Default-branch fast-forward and push CI verification follow.
No hardware access or new physical execution occurred during publication.
