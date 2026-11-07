<!-- SPDX-License-Identifier: MIT -->
# Independent code review

## Terra: R1 — blocking nested manager lock (2026-11-07)

Status: reproduced in isolated host model; Astra response pending.
Source: `source/gpu-next-worktree/src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp`,
`get_aperture` lines 418–423 and guarded callbacks starting at line 1118 in the
review snapshot. Each guarded callback acquires `sLock`, then calls
`get_aperture`, which acquires the same non-recursive semaphore lock.
Jamfile selects `headers/private/graphics/intel_extreme/lock.h`.
The second acquisition blocks before lookup, including when the ID is invalid.

Independent reproducer:
`python3 /home/dan/renkuos-terra-n100/research/terra-n100/check_lock.py`

It compiles the copied production lock header, helper and query callback.
The semaphore stub detects entry to the blocking path (exit 42); removing only
helper acquisition in a temporary control makes the callback complete (exit 0).
No production file was modified. This is host-side control-flow validation,
not a kernel execution test. Existing manager/threaded fixtures mock the helper
without its lock and therefore omit this defect.

Recommendation for Astra: retain lifetime protection with exactly one manager
acquisition; use a caller-locked lookup helper or direct lookup, audit all callers,
and include the actual helper in the lifetime fixture. Do not remove the outer
lifetime guard without an equivalent ownership mechanism.

Full review and other findings are being recorded in
`/home/dan/renkuos-terra-n100/GPU_DRIVER_REVIEW.md`.

### Astra disposition

Pending; Terra has not modified or integrated production code.

### Astra response to R1 (2026-11-07)

Accepted, blocking. Removed nested acquisition from get_aperture; kept all seven outer lifetime guards and documented caller-lock requirement. Updated gart-threaded-host.py to extract the actual helper instead of substituting an unlocked mock. 100 threaded ASAN/UBSAN rounds pass. Native revalidation pending; Terra please independently validate against current production source read-only. Earlier fixtures omitted this lock, so earlier native/host results do not close this finding. No hardware deployment authorized by those results.

### Astra validation and remaining dispositions

Build 073 passes native compilation, bundled tests, 17 recovery cases and a new
native fixture containing the actual get_aperture helper and query callback,
compiled against the production lock.h with real RenkuOS semaphore operations.
Both success and missing-ID queries return; counts balance; five-second alarm
bounds deadlock. Host finalizer passes and VM shuts down cleanly. Evidence:
installation/private/gpu-hdmi-build-050/native.log, finalize.log, validation/.
Terra R1 independent verification still requested; native source was not loaded
as a hardware driver. Earlier mocked-helper success is insufficient evidence.

R2 accepted as blocking: OpenHashTable::Insert returns B_NO_MEMORY when initial
bucket allocation fails. Both ignored publication results confirmed. Next fix:
check registry insertion before returning aperture ID/base; unwind linked memory
metadata on initial hash failure before any backing allocation. Require fault
injection at initial bucket allocation and untouched output assertions.
R3 accepted: lock resource destruction is missing; constructor failure and final
use ordering must be covered before adding uninit_lock.
R4 accepted: agp_uninit still uses resizing Remove during iteration; expanded
registry test needed, not just Intel singleton tests.
R5 accepted as a submission blocker: external-area lifetime must be pinned or
excluded from execution. Retaining GART metadata is not retaining borrowed pages.
No physical validation or GPU submission will rely on unresolved R2/R5.

### Astra R2 correction ready for independent review

Patch 0076 checks both hash Insert results. map_aperture publishes output only
after successful registry insertion; failure deletes the aperture while owning
the backend module reference. CreateMemory unlinks/deletes its record on hash
failure before backing allocation. Tests: gart-recovery-host.py now has 18 cases
including registry publication failure and unchanged output; gart-publication-host.py
exercises production CreateMemory/_Remove with 100 failed hash publications then
successful retry. Both reject build 073 source. These inject the Insert failure
return, not actual allocator exhaustion in the kernel or real hash allocator.
Native build 074 running; Terra please verify error paths and assess need for a
real OpenHashTable initial-bucket allocation fault fixture. R2 not independently
closed. No changes to Terra's worktree or production hardware.

## Terra R1 revalidation (2026-11-07)

Accepted correction independently checked. The captured corrected source keeps
all seven outer guards and removes helper reacquisition. Own harness compiles
production lock/header/helper/query and completes. Updated Astra actual-helper
threaded fixture independently passes 100 ASAN/UBSAN rounds. Running that same
fixture on the old double-lock snapshot fails within its bounded gate, as expected.
Evidence and hashes: `/home/dan/renkuos-terra-n100/research/terra-n100/`:
`lock-fixed-check.log`, `threaded-correction.log`, `threaded-negative-control.log`,
`evidence/correction-validation.json`. Host validation closes R1 regression only;
native/kernel and physical gates remain open.

## Terra additional review R2–R5 (Astra disposition pending)

Full paths/line references and snapshot hashes:
`/home/dan/renkuos-terra-n100/GPU_DRIVER_REVIEW.md`.

- R2 HIGH: CreateMemory and map_aperture ignore initial hash Insert B_NO_MEMORY.
  Successful-looking handles can orphan records/backing/backend singleton/module
  ownership. Inject first hash-bucket allocation failure, not just record OOM.
- R3 MEDIUM: aperture destructor and manager uninit omit semaphore destruction;
  repeated reopen/unload leaks kernel semaphores. Check initialization unwind too.
- R4 MEDIUM, conditional multi-aperture path: agp_uninit removes with shrinking
  hash Remove while iterating. Use iteration-safe removal; expanded-table test.
- R5 HIGH integration concern, conditional external backing: failed partial bind
  plus failed rollback retains PTE prefix without owning/pinning caller area or
  returning recovery token. Audit caller release-on-error; required before DMA.

These are snapshot source findings; R2–R5 native failure injection was not run.
Terra did not edit production source. Astra owns correction/disposition.

R2 native follow-up: build 074 and finalizer passed, including native registry Insert fault and real-lock tests. The separate memory-record Insert failure test remains host-only. Independent Terra disposition pending.

### Astra R3 aperture-lifetime correction (partial)

Patch 0077 adds uninit_lock in Aperture destructor only when fLock.sem >= B_OK,
after the empty-memory assertion and before backend/module release. Host fixture
extracts actual destructor: 200 valid/failed-lock cases check release ordering.
Build 074 source fails the runtime regression. Semaphore/backend calls are mocked;
real kernel resource accounting is not claimed. Native build 075 running.
Manager lock/hash destruction and init unwind remain open with R4 iterator safety.
Terra validation requested; this does not close all of R3.

Build 075 passes native compilation, bundled tests, 18 recovery cases, 2 real-semaphore query cases and host finalizer. VM clean shutdown. Patch 0077 aperture semaphore cleanup covered by 200 host-only destructor cases; previous source fails negative control. Manager-level R3 and R4 remain open; independent Terra validation pending. No physical hardware test. Evidence installation/private/gpu-hdmi-build-052/.

## Terra R2 independent validation (2026-11-07)

Reviewed corrected CreateMemory and map_aperture from patch 0076 in captured live
source. Ownership rollback/output publication now follows R2 recommendation.
New independent real-OpenHashTable test passes 100 initial bucket allocation
failures plus retry/cleanup under ASAN/UBSAN. It uses actual CreateMemory/_Remove
and hash header, with modeled placement/OS services; old source fails its negative
control. Updated Astra map/unmap recovery fixture independently passes 18 cases;
registry Insert there remains a modeled return failure. No actual kernel allocator
exhaustion or physical test by Terra. Source/host R2 finding closed; native report
074 belongs to Astra and was not independently rerun here.

Reproducer: /home/dan/renkuos-terra-n100/research/terra-n100/check_hash_publication.py
Evidence: hash-publication.log, hash-negative-control.log, r2-map-recovery.log,
evidence/r2-validation.json. R3/R4 corrections and R5 lifetime design remain open.


### Astra R4 correction and real-table regression

Patch 0078 uses RemoveUnchecked in agp_uninit. gart-uninit-host.py extracts the
production teardown and compiles the actual OpenHashTable.h/TypeOperation.h,
with mocked OS/backend cleanup. A 256-object expanded table is completely removed;
failure at object 128 preserves PCI ownership and remaining records for retry.
Current source passes ASAN/UBSAN; build 075 source fails the same regression.
Native build 076 running. No real kernel teardown or physical GPU test claimed.
Terra independent validation requested. R3 manager lock/init unwind is still open;
hash placement construction/global destruction ordering needs source verification
before adding an explicit destructor or reconstructing a global table.

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

Astra physical validation review request: physical/memory-smoke.cpp and physical/MEMORY_VALIDATION.md. Bounded 30 serial owned non-reserved allocate/free cycles, exact returned descriptors, no mapping/command writes. Native compile and --help only underway; no physical invocation. Review admission, ownership on failure and recovery gates before A3. Existing Fedora-specific preflight script is stale for this local workflow and has not been run.

## Terra independent R3/R4 and design review — 2026-11-07

Reviewed live manager SHA256 ae4171b8ef0f200472dc9f15c104627b5a2e7c2d48da565a01f384fe644bcced. Independently ran gart-lock-release-host.py: 200 production destructor valid/failed-lock cases pass; gart-uninit-host.py: 256 real-hash records, failed cleanup/retry and init/unwind pass. Both model OS/backend services; no independent kernel unload or physical validation. Source review supports 0077–0079: destroy only initialized aperture locks after last guarded use; partial teardown preserves remaining registry/PCI/manager lock; RemoveUnchecked avoids iterator resize; successful teardown destroys and reconstructs the table before destroying manager lock and releasing PCI. R3/R4 source and host disposition: acceptable; native/kernel teardown stress remains open. The first attempted invocation omitted the fixture's required root argument and failed before running; corrected invocation passed.

EXECUTION-OWNERSHIP.md: directionally acceptable for design development, not approval for implementation or hardware execution. Critical unresolved contract: acquire/release must atomically retain device/request lease across dropping the mutex and during every GART call; device shutdown must be serialized with those references. Define how owner acquires an already active display aperture without duplicate singleton creation or invalidating display pointers, and ensure retained workers/IRQ teardown and callbacks do not reference unloaded legacy code. Never reclaim known or hidden allocations until all display leases and GPU DMA are quiescent. R5 remains open for borrowed buffers; owned-only exclusion is valid for proposed workload, contingent on a real owned DMA/page-table API and partial-bind recovery.

Memory smoke/procedure: conditionally acceptable as a bounded allocation/free smoke only after documented identity, matched artifact, active-display and recovery admission gates are independently evidenced. Verified device.cpp:320–367 uses returned allocation base for free and rolls back failed copyout; hidden partial-bind and failed-free ownership remain possible. Important limitation: diagnostic trusts the operator-provided /dev/graphics path and shared ABI check; it does not itself verify PCI ID, revision, loaded artifacts, or that another display lease remains live. Admission must prove these externally immediately before invocation and prevent last-close teardown; opening the node alone is insufficient. A process alarm does not bound a stuck kernel ioctl. On failure stop, preserve descriptor/logs and avoid guessed cleanup. No physical invocation by Terra.

### Astra disposition and T3 revision

Accepted Terra R3/R4 source/host validation; kernel/physical stress is not closed.
Accepted memory-smoke conditional review and external admission requirements.
Accepted T3 atomic lease and existing-aperture objections. Appended concrete
operation-pin/Closing/quarantine protocol and cold-boot-only ownership conversion
to EXECUTION-OWNERSHIP.md. No hot adoption, duplicate aperture, or legacy callback
references permitted. Corrected A-F2: NEED_PHYSICAL already excludes reserved
placement in current _Insert; both flags remain explicit proposal. Terra follow-up
review requested before production implementation.

### Astra second T3 disposition and test request

Accepted all five blockers. Added explicit conservative error-to-publication-block
transition, permanent single-device record/nonreused numeric lease IDs, no unsafe
same-boot display reopen, sole module MMIO mapping, and no unproven in-place
quarantine recovery to EXECUTION-OWNERSHIP.md for further review. These initial
limitations do not satisfy the final stable desktop/recovery goal. No production
owner implementation yet. Expanded threaded callback regression passes 300 host
rounds and detects independently omitted query/allocate/free guards. Terra review
requested; evidence testing/gart-threaded-300/. No physical test.

## Terra patch 0080 independent review — adapter blockers

Reviewed IntelApertureLeaseState.h SHA256 11c73f2a78a3db0f2987e3ad666fdefcf2e2e847e9586651f4ce240c852a0735. Independent source-derived reproducer: /home/dan/renkuos-terra-n100/research/terra-n100/lease-review/probe.cpp. Host execution only.

1. **Initialization cannot perform forwarded GART operations.** BeginInitialization creates the display lease and enters Initializing; BeginOperation rejects *all* accesses in that state. Existing intel_extreme_init allocates and queries GART before its successful return. An adapter cannot complete initialization through the mandatory pinned forwarding path without changing ordering or bypassing lifetime tracking. Define and test initializing display-only query/allocation/free admission, including allocation failure concurrent with lease release and FinishInitialization success/failure. Do not transition Ready before the driver actually finishes initialization merely to bypass the guard.

2. **An allocation failure does not prevent final cleanup of potentially hidden backing.** EndOperation(true) only sets fDiagnosticBlocked. After Release(display), BeginCleanup(true,true) succeeds even though GART's failed rollback can have hidden retained pages; this contradicts the agreed no in-place quarantine retry/reclamation policy. Source-derived probe reproduces Success. Track unresolved allocation cleanup separately from diagnostic admission, prevent cleanup until a separately proven recovery contract exists, and test display and request allocation failures. Merely passing quiescence booleans cannot establish hidden metadata ownership.

The existing test actually asserts cleanup succeeds after allocation failure, so update the expected invariant, not only implementation. Test invalid Access values and ticket destruction/lifetime contract at adapter boundary as additional hardening. These findings concern state logic only; no GPU or physical hardware operation performed.

### Astra patch 0080 implementation review request

Isolated IntelApertureLeaseState.h and tests/intel-aperture-lease-test.cpp now pass
host sanitizer/negative-control tests and native userland state tests in build079.
Please review invariants before adapter integration. External synchronization is
explicit; caller must retain actual backend/module/MMIO resources. No integrated
retained module or GPU publication exists. Native kernel behavior is untested.

### Build 079 and adapter lifecycle follow-up

Astra reports native build 079 compiled and passed bundled tests including state harness, 18 recovery and two real-lock cases, diagnostic help-only and finalizer; VM shut down cleanly. Terra did not independently rerun that native build. No kernel module load or physical validation is established, and the two patch 0080 state blockers above remain open.

Adapter review must include intel_extreme.cpp:666 (MMIO area creation), :775 (shared-info area ID), :969 (interrupt generation teardown), and :995 (unconditional area deletion), plus every accelerant clone consumer. Transfer mapping ownership without deleting an area still owned by the retained module; define interrupt ownership/masking so legacy last-close does not disrupt any future module-owned GT activity. Test normal and failed init, clones, last close, and unload with retained resources. Do not assume area-ID or IRQ lifetime follows the lease automatically.

### Astra correction of both 0080 blockers — patch 0081

Accepted both defects, including the incorrect original test expectation.
Initializing now admits display-only forwarded query/allocation/free; finish
returns Busy with active tickets, release remains Busy until initialization is
explicitly finished. A successful finish cannot clear prior allocation failure.
New sticky unresolved-allocation state prohibits final cleanup even after all
leases/pins are gone and both quiescence inputs are true. Known-handle display
free remains permitted. No unsafe hidden-backing reclamation path is exposed.
Invalid Access rejected; adapter ticket completion obligation documented.

Updated host sanitizer suite passes transitions +100 races, four initialization
success/failure combinations, and display/request allocation failure retention.
Old0080 source fails new invariant; individually removing init admission, hidden
retention, or sticky failure also fails at runtime. Evidence testing/intel-owner-002/.
Initial test compile missed initializer_list include; corrected before tests ran.
Native080 running. Independent validation requested; no backend or physical test.

## Terra patch 0081 independent disposition

Reviewed current IntelApertureLeaseState.h SHA256 90a749760d30e578a428fbbcdf431ff0f8d6e38524e1acec6c4dfbe8ba4d623d. Both 0080 adapter blockers are resolved in the state-only core: display operations can be pinned during initialization and initialization cannot finish while they are active; sticky unresolved-allocation state survives successful init and prevents cleanup despite positive quiescence flags. Invalid Access values are rejected. Independently compiled and ran the current state fixture with ASAN/UBSAN: transition suite and 100 close/operation races pass. An independent targeted probe covers init allocation, blocked finish, failed allocation, successful finish and blocked cleanup: /home/dan/renkuos-terra-n100/research/terra-n100/lease-review/probe-0081.cpp. An earlier probe designed for the old state machine was not a valid post-fix check because it left the new init operation pinned; it was replaced with the explicit assertions above.

Disposition: source/host validation closes the two reported 0080 defects and supports review of a staged adapter. It does not establish correct backend, module, MMIO, IRQ, cloned-area, or kernel lifetime integration. Adapter must invoke EndOperation on every outcome under mutex with allocationFailed set for every allocation failure, hold owner/module dependencies, and provide actual quiescence evidence; review all lifecycle paths and the previously recorded publish/failure race before GPU execution. Native080 is Astra-reported as running, not independently validated here. No physical test.

## Terra preliminary patch 0082 retained-owner review

Reviewed IntelMemoryOwner.cpp SHA256 305d3d30679d0b8a8bf48ee907d71ee057f3ab1155b7ed505b526d1f68d45695 and interface. Module takes PCI/GART references, uses a permanent state object, creates the display lease before acquisition, uses pinned RAII for Query/Allocate/Free, ends pins under the mutex, and intentionally retains resources on final display release. No backend fixture or live module run by Terra yet.

**Adapter integration blocker — failed Free is not reflected in owner state.** Free returns sGART->free_memory status without marking any persistent failure. A failed unbind leaves backing/GGTT metadata retained; yet owner remains Ready, diagnostic-block state remains unchanged and subsequent allocations remain admitted. This conflicts with the approved failed-cleanup quarantine contract and would allow future diagnostic preparation to overlook unresolved retained state. Record cleanup failure under the owner lock before dropping the operation pin and block diagnostic admission and final cleanup; specify whether future display allocations are allowed and preserve valid cleanup/query operations. Add injected failed-free tests including a concurrent release/preparation and assert pin + sticky quarantine behavior. Since the current adapter exposes no request or hardware submission, this is a future integration/lifecycle correctness blocker, not a claim of a present GPU execution failure.

Additional checks before wiring: inject every module acquire/map/query/MMIO failure, including map failure due to another active owner; verify first caller alone publishes outputs and later acquires fail closed, no premature delete/unmap/put and exact module reference accounting. Gate close while GART call is blocked, stale/duplicate lease use, init finish with active operation, failed copyout handling at driver integration, and MMIO clone/IRQ ownership. map failure currently permanently transitions to RetainedDisplay even with no owner resources: document this conservative reboot-only denial and verify that it does not touch the other owner's aperture. Verify BAR is memory, address/size valid and same cache/clone attributes as legacy mapping. Existing intel_gart retains its separate mapping; any claim of a sole global MMIO alias would be inaccurate.

The publication-versus-concurrent-failure ordering remains a prerequisite for a later submission implementation; this adapter has no such publication path. No physical execution or deployment.

## Terra independent patch 0083 disposition

Reviewed owner SHA256 f521ca7b9b0ec08d8c4122c2bb9ec675281f2cfc80fca24aedf09e950196909b and state header SHA256 14c12bcbc5541035367f2d7d6870a485001b0e5ddd93c0fe30216f624cf3bdd1. Failed Allocate and Free both set sticky unresolved backing in the RAII destructor under mutex before the operation pin is dropped; a subsequent successful Free does not clear it. This blocks diagnostic admission and BeginCleanup, while allowing display query/free and later allocation as specified. BAR rejects I/O, zero, unaligned and overflowing ranges.

Independently ran intel-owner-adapter-host.py against current actual module under ASAN/UBSAN: modeled initialization, forwarding and fault fixtures passed, including free failure, later success, blocked diagnostics and cleanup. The earlier failed-free source/host blocker is resolved. Fixture mocks kernel/PCI/GART services and exercises serial calls; it does not establish real module lifetime, concurrency, cloned areas, interrupt handling, real BAR semantics, or hardware safety. Native083 reported compiling and not independently run here.

Before driver wiring acceptance: exercise real adapter operation pin versus release with barriers and faulting backend; module-init dependency balancing, driver error unwind, clone area/IRQ teardown, actual physical BAR/PCI identity, and preservation of existing display. Verify all failed frees call the owner (no direct GART bypass). Hardware admission, DMA/cache/TLB, and publication-order gates remain open.

## Terra adapter concurrency fixture review

Independently ran actual-owner fixture SHA256 afd8517f16a931aaf14a2363af508016630c90b00b1fdf602cca8ba11ead2290 under ASAN/UBSAN: all 300 Query/Allocate/Free success/failure versus release barrier rounds and initialization finish/release barriers passed. Reviewed its condition-variable gates: each forwarding call is held inside the modeled backend after the actual adapter has acquired its pin; release occurs before backend return; cleanup must be busy until pin release, and failed mutating calls leave sticky cleanup blocking. Initialization tests check FinishDisplay success/failure and release cannot pass an active allocation. Real host mutexes are used; backend, PCI, mappings and module references are modeled. Premature/leaked-pin negative controls are not claimed independently verified.

Disposition: concurrency test supports proceeding to review of a staged wiring change; it does not itself approve wiring or hardware deployment. Remaining pre-integration review covers actual display init/error unwind/last close, clone areas, IRQ and MMIO ownership, cross-module lifetime, stale leases, and physical compatibility. Future GPU publication still requires serialization with concurrent backing failure and DMA/cache/TLB/reset evidence. No native/kernel or physical concurrency test by Terra.

## Terra preliminary patch 0084 driver wiring review

Reviewed intel_extreme.cpp SHA256 add9c4fe0c3b93e3028d584806f9ead459af4ce69bc144a345b580070b85cbbd, private header and actual accelerant clone call sites. N100 rev0 takes begin_display resources; allocation/free go through owner; other hardware follows legacy mapping; successful init calls finish_display before publishing success; last close disables/removes legacy handler before release; owner MMIO area is not deleted by the legacy driver. Accelerant clone_area uses shared_info->registers_area (accelerant.cpp:134–136), so preserving a valid cloneable area ID and its lifetime is essential. Current adapter has no GT handler/request submission, so global interrupt disable is not currently evidence of a GT regression; any later module IRQ use must coordinate masks and handler ownership. No concrete additional unconditional failure established in this source review.

Integration acceptance pending focused behavioral tests, not just release helper extraction: inject begin_display/get_module failure; shared-area creation failure after begin; driver initialization failure and finish_display failure, including pending ticket B_BUSY and whether release_aperture can ever drop the only reachable lease after failed finish/release; validate no unsafe area delete, module put or second map. Exercise last close with active forwarded operation and ensure no use of shared_info or driver IRQ callback after deletion. Test existing accelerant register-area clone, clone close and module-backed area after legacy teardown on a native isolated setup; verify IRQ install/remove and MSI balance for success/fallback/error. Test non-N100 legacy path separately and reopen B_BUSY limitation. Check adapter's MMIO mapping timing versus legacy PCI D0/command enable and cloned-area protections on actual target before claiming display compatibility.

Do not assert that pinning the owner area alone pins accelerant/shared-info clones or the legacy IRQ handler. No hardware access by Terra; native084 remains Astra-reported compiling. Physical testing still requires live recovery connectivity and reviewed isolated boot.

## Terra independent whole-driver lifecycle fixture review

Independently ran driver-lifecycle-host.py SHA256 8e23e88a0315f69a8213769356d8a3948d3fcf824707bddf3c9a8c1d1708fd30 against current source under ASAN/UBSAN: 36 extracted production init/uninit cases passed with modeled services and symbolic registers. The fixture includes both N100 and legacy paths and expected IRQ/MSI teardown ordering. This independently confirms those fixture results, not real kernel concurrency, MMIO, clone-area or display behavior.

**Conditional ownership gap in finish-B_BUSY unwind:** intel_extreme.cpp:1010–1015 calls intel_extreme_uninit if finish_display(true) fails. release_aperture :587–600 then attempts finish(false) and release; if both return B_BUSY it still clears the only driver-side owner pointer/lease and puts the client reference. The module remains loaded and resources retained, so no premature DMA free is established, but the live initializing lease can become unreachable and the owner permanently rejects reopen. Fixture fault stage 6 intentionally returns B_BUSY for both calls yet only counts attempted release and zero caller refs; its model does not track whether the lease was actually closed. Determine whether an active ticket can occur at this point in the real driver; if impossible, establish and test the invariant; otherwise preserve an actionable module-owned pending-release record or explicit recovery status until the active ticket ends. Do not silently call this a successful release. This is a conditional lifecycle concern, not a physical failure report.

Independent clone support: created /home/dan/renkuos-terra-n100/research/terra-n100/native-owner/area-clone-probe.cpp, SHA256 d2f7ab15f7f07d3fcec21cfe9c5c2e0d270ca7930fa533e74cfd454e9301c7f3. It creates two same-team RAM area clones, deletes owner then one clone, and checks remaining visibility and cleanup. Host Linux stub with ASAN/UBSAN passes, validating probe mechanics only; no native VM run. RenkuOS vm.cpp vm_clone_area checks B_CLONEABLE_AREA for userland cross-team clones, so same-team success must not be generalized to the actual accelerant cross-team kernel-MMIO case. Native VM and cross-team/real module tests remain open.

### Astra disposition of full-lifecycle fixture review

Accept scoped 36-case source/host evidence and conditional Busy unwind concern.
Not accepted as successful live-lease release. Proposed explicit initializing
lease revocation: transition RetainedDisplay under module mutex, keep operation
pins/backing, finish late operations without reopening. Review requested before
implementing the changed release contract. No driver integration acceptance.

## Terra review of initializing lease abort proposal

Proposed AbortInitialization is acceptable for an isolated implementation and independent test review: revoke only the matching live initializing display lease while holding the same mutex; enter RetainedDisplay, prevent new operations/reopen/diagnostics, and retain aperture, MMIO, record, dependencies and each existing operation ticket. EndOperation must find the revoked lease until all tickets finish and set sticky unresolved state on late mutating failure. A successful release response means revocation accepted, not hardware quiescence or memory cleanup.

Before integration test both completion orders: backend success and failure after abort; blocked Query/Allocate/Free with concurrent finish(true), finish(false), abort and repeat release; stale ticket/lease and wrong-kind rejection; no post-abort operations, no initialization success after abort, no cleanup while pinned and retained display/quiescence unknown; module code/dependencies retained after driver put. Check success/failure of init resource acquisition before the lease is returned separately. Quiescence flags alone must not authorize reclaim after an unknown late backing failure. This does not solve same-boot reopen or GPU publication ordering. No physical testing.

## Terra patch 0085 independent validation

Reviewed state header SHA256 adea0412f14056b4cf054707e93accfbb3f37dc8dbcb56ea2311ec51f34dfdd4 and module SHA256 668dd25e6e6ff9d7560d38590f2bbf10efb30d0ac003b5d8772863a97a7079fc. Independently ran current state harness and actual-adapter fixture under ASAN/UBSAN; state transitions and 100 races passed, as did 300 adapter races and added init-abort cases. Source and tests show ReleaseDisplay revokes only a matching initializing display lease on Busy, retains existing pins, blocks new operations and diagnostics, and keeps sticky unresolved backing on late failed allocation/free. Wrong/stale ID, repeat release and finish after abort are rejected. This resolves the reported conditional orphaned-live-lease issue at source/host scope. Success means revocation, not resource reclamation.

Adapter fixture also permits internal BeginCleanup(true,true) after a successful late operation; current module exposes no cleanup/retry API. The adapter must not turn those test booleans into a claim of verified display/GT idle. Driver wiring, IRQ/shared-area, actual kernel module lifetime and physical behavior remain unverified.

### Astra 0085 disposition and native test step

Accepted independent source/host validation. The module revokes the matching
initializing lease on release while retaining all late-operation pins; it does not
claim hardware idle or reclaim resources. Old release and two broken abort variants
fail new runtime assertions (testing/intel-owner-abort-001/). Driver lifecycle
fixture now asserts no live caller lease remains after its finish-B_BUSY model.
Copied Terra's reviewed same-team RAM clone probe to tests/area-clone-probe.cpp
for native085 execution; no cross-team/kernel-MMIO claims. Native run underway.
Terra preparing isolated kernel module-load/RAM-clone probe source; Astra reviews
and owns any disposable-VM execution. Physical gates remain closed.

## Terra independent IRQ / MSI source review

Confirmed Astra's preexisting failure paths in intel_extreme.cpp:427–475, 562–575, 1025–1053. If configure_msi succeeds but enable_msi fails, the short-circuit expression loses configured state and never calls unconfigure. If enable succeeds but sem ownership or handler installation fails, fake_interrupts becomes true and sem becomes B_ERROR, so uninit skips MSI disable/unconfigure. A successful handler with valid semaphore ID 0 is also skipped by the >0 teardown predicate. Depending on MSI/vector reuse and late interrupts, this can leave resources or a handler live across driver teardown; physical failure has not been reproduced.

Additional no-vector path: when no MSI is usable and the legacy vector is 0 or 0xff, status remains B_OK and the handler-install block is skipped; the fake-interrupt fallback only checks status < B_OK. Inspect initial fake_interrupts state and ensure no-vector is explicitly represented; do not infer installed-handler state from a semaphore ID. If no handler was installed, uninit must not remove one or mask/unmask as though it had been. Also audit successful semaphore ownership/deletion separately: init transfers vblank semaphore to the calling team, but normal uninit in the examined source does not delete it. Determine intended accelerant/team lifetime and avoid double-delete or leaving users with a stale semaphore.

Fix should track sem-created, MSI-configured, MSI-enabled and handler-installed separately; unwind in reverse order on each failure, including sem-owner failure before any avoidable PCI setup, enable failure after configure, handler install failure after enable, no vector, valid sem ID 0, legacy and MSI fallback. Preserve MSI resources if disable/unconfigure itself fails rather than claiming successful cleanup; record diagnostics and test supported return semantics. Mask relevant source before successful handler removal; only delete callback/shared state after removal/drain succeeds. The existing 36-case lifecycle fixture mocks init_interrupt_handler and cannot validate these internal paths; add an extracted actual-init fixture with backend fault injection and negative controls.

Kernel source f01e0b78 src/system/kernel/interrupts.cpp:277–299 and :524–544 shows dispatch holds vector_lock across callback and removal takes that lock. For a vector with no_lock_vector == false, successful removal serializes against active callbacks. The display handler passes flags 0, but no_lock_vector is a per-vector property; verify no other registration sets no-lock on the shared vector before relying on this as a universal drain guarantee. A failed removal is not a drain. Native concurrent IRQ tests and physical validation remain required.

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

## Terra IRQ fixture and PCI MSI contract review

Reviewed irq-init-host.py: it executes actual init_interrupt_handler with modeled PCI/sem/handler calls; the reported baseline incomplete-resource cases are valid within that model. Its predicates are useful for detecting configured-without-handler, missing fallback and unused semaphore, but it does not run production uninit, test semaphore ID 0, multiple-generation paths, real interrupt vector synchronization, or inject failures into disable/unconfigure. Symbolic MMIO constants cannot prove register behavior. Expand alongside the fix and add runtime negative controls for each independent resource state and teardown ordering.

Source reference RenkuOS f01e0b782747076cb18d6c9f7952bcc358ee87e8 src/add-ons/kernel/bus_managers/pci/pci.cpp:2193–2237, 2240–2270, 2275–2336: ConfigureMSI returns B_BUSY on an existing configuration and may fail vector allocation; UnconfigureMSI returns B_UNSUPPORTED or B_NO_INIT before freeing vectors when not configured; DisableMSI returns B_UNSUPPORTED or B_NO_INIT before clearing enable when unavailable/not configured. Both APIs return status_t, so the driver must not blindly treat failures as successful resource cleanup. Successful EnableMSI also sets PCI_command_int_disable; the examined DisableMSI does not restore that command bit. If a fix intends to fall back to legacy INTx *after MSI has been enabled*, it must explicitly establish and restore the correct PCI command state and test it; simply calling disable/unconfigure does not prove INTx is re-enabled. Avoid touching unrelated interrupt-enable state without a documented ownership contract. Source review only; no native fault injection or hardware validation.

## Terra scoped IRQ prerequisite review

Independently ran irq-init-host.py --early-paths against staged source SHA256 af092890dd2b88027352b0830adc88e1e846d0f5758a86b58d84190303807469 and reviewed the changed initializer/uninit. Initial fake/no-IRQ defaults, early return after sem-owner failure, explicit no-vector error, and accepting semaphore ID 0 correct the identified early cases at source/host scope. The focused suite exits 0 but explicitly still reports 12 modeled resource/fallback failures in the full matrix, including MSI cleanup and failed removal/disable. These remain blockers; this is not an IRQ lifecycle acceptance or physical result. Source review confirms PCI UnconfigureMSI frees vectors while DisableMSI is separate, so on failed disable do not unconfigure a potentially enabled source. Preserve handler/shared state on failed removal under a supported lifetime contract.

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

## Terra scoped MSI correction review

Reviewed staged intel_extreme.cpp SHA256 99f488ca1f10e52f3152ac5e0404e67658a5d31e849638f16fd5acf9f906d95d. Independently ran actual extracted irq-init-host.py --msi and full-driver lifecycle fixture: MSI focused cases passed and 36 modeled lifecycle cases passed. release_msi now separately tracks configured and enabled, refuses to unconfigure after failed disable, retains failed configuration for B_BUSY reinit, and only uses the legacy line after a failed MSI enable plus successful vector release. Enabled-MSI handler failure falls to fake mode rather than assuming INTx was re-enabled. These close the previously identified scoped MSI ownership paths in source/host model.

Full suite still reports eight modeled IRQ lifecycle issues; sem lifetime, removal failure and retained legacy handler/code lifetime are not closed. Existing tests model PCI functions/IRQ/MMIO, not actual kernel or physical N100. Check B_KEEP_LOADED ownership of unresolved callbacks before driver image unload; do not count a failed handler removal as drained. Astra reports native088 pending; Terra did not run a native or physical IRQ test.

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

## Terra recommendation on IRQ removal invariant and semaphore ownership

For the isolated development candidate, a documented fail-stop panic before freeing callback state or allowing legacy image unload is defensible *only* if the driver first tracks the exact successful install (vector, handler and info), serializes teardown, proves the vector uses the locking dispatch path, and treats unexpected remove failure as an invariant violation. This is a development containment choice, not stable-desktop recovery and not physical deployment approval. Kernel legacy_drivers.cpp:463–476 ignores uninit_driver result and unloads the image; a local early return or logging cannot retain callback code. Long-term acceptable recovery requires an actually supported lifetime owner for callback code/data or a corrected driver-manager unload barrier; moving callbacks to a retained module requires its own IRQ/MMIO/locking design, not merely retaining a pointer.

Successful remove serializes callbacks only when the vector's no_lock_vector is false. interrupts.cpp:504–512 documents B_NO_LOCK_VECTOR as non-shared but does not enforce exclusive registration, and the flag is per-vector; flags 0 on this driver alone are not complete proof for a shared INTx vector. Establish MSI vector allocator exclusivity and the legacy-vector precondition, or provide a stronger drain mechanism. Mask generation before remove and do not free state on a failed/unproven drain. Test deliberate remove-failure and no-lock/shared-vector models without claiming injected failure reproduces actual hardware.

Private kernel vblank sem ID is the right source of truth; shared_info is writable by clients. sem.cpp:352–381 checks the exact ID under locks and B_BAD_SEM_ID can mean team cleanup already deleted it. After handler drain, tolerate B_BAD_SEM_ID only when the ownership/lifetime contract accounts for team death; test ID 0, owner death, already deleted ID, close while callback runs and duplicate close. Prove the kernel's ID generation/wrap and owner checks before assuming a stale ID can never identify a reused semaphore; do not interpret an arbitrary error as safe deletion. Preserve the current shared ABI as a mirror only. No physical IRQ validation or production source modification by Terra.

## Terra scoped 0088 private-state review

Independently ran irq-init-host.py --private-state against staged source SHA256 e32ce739f36af66dea0f2c2023453549be5b875c32a81c70a62dabfbb47adcb1; focused private semaphore/handler tampering tests pass with modeled services, alongside early and MSI checks. Source now retains kernel-owned vblank semaphore ID and installed-handler flag; the user-writable shared field is only a mirror. This resolves the specific trust/teardown-predicate concern at source and host scope, not general IRQ safety. Full matrix still reports eight modeled issues; successful semaphore deletion, failed handler removal, callback drain, and retained legacy code/state remain open.

Teardown recommendation remains as previously documented: fail-stop only as an explicitly temporary development containment for a proven impossible-to-recover invariant, before unsafe resource release; it is a kernel crash and not stable recovery. Prefer a supported unload barrier or retained-module-owned callback/data for production, after resolving shared vector/no-lock semantics. No kernel or physical IRQ run by Terra.

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

## Terra independent interrupt-vector allocator candidate validation

Independently ran irq-vector-host.py against captured production allocator: 2,048 modeled occupancy/count cases, 255 failures; against a separate temporary copy changing only `runFound` initial value true to false: 2,048 cases, zero failures. Evidence /home/dan/renkuos-terra-vulkan/evidence/irq-vector/{baseline,candidate}.log. Confirms full-table/no-free-run bug and proposed one-line correction for modeled NUM_IO_VECTORS=8 and positive counts 1–8. Source code and kernel not modified by Terra. This does not establish native vector allocator/interrupt behavior, count 0, negative or oversized-count handling, saturation on real system, SMP behavior, or architecture side effects. Validate input-range contract separately; do not expand the candidate's claim beyond tested positive counts. Synthetic IRQ probe remains gated pending kernel-owner review and export/side-effect checks.

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

### Terra: interrupt vector no-lock flag can persist across reuse

In `src/system/kernel/interrupts.cpp:191-196`, boot initialization sets `no_lock_vector` false; `install_io_interrupt_handler` at 504-512 can set it true. Handler removal (524-578), vector allocation (638-680), and free (690-718) do not clear it. Dispatch at 277-278 and 355-356 skips the vector lock when true. Therefore an allocation and registration with flags 0 do not alone establish the lock-based callback drain property after a vector has previously been marked no-lock. HPET installs a no-lock handler at `src/system/kernel/arch/x86/timers/x86_hpet.cpp:280-283`; whether its vector can be subsequently freed and reused in the tested configuration remains unverified. Do not set the flag as a probe on a reusable vector. Test needs proven pristine vector history or explicit instrumentation; resetting the flag safely requires synchronization analysis. Astra notified.

### Terra: independent review of candidate kernel boot evidence

Reviewed `gpu-driver/tests/evidence/kernel-vector-boot-001.json` and recalculated all four recorded evidence hashes: all match. `installation/private/kernel-boot-002/verified-serial.log:277` contains the unique candidate revision; `package-comparison.json` reports 6204 entries compared with only the kernel changed, and matching candidate hashes. Verified result and boot log record clean shutdown. The procedure documents earlier failed verifiers, the userland uname revision, package attribute comparison limits, and VM-only scope. Evidence supports the stated disposable VM boot checkpoint; Terra did not independently rerun the VM or package extraction. It is not IRQ stress or physical GPU execution evidence.

### Terra: proposed native IRQ drain test design review

The removal-entry hook fires before vector-lock acquisition. A remover can publish its entry event, be descheduled, and resume only after callback exit. The proposed event ordering therefore establishes function-entry overlap but does not establish actual lock contention or that removal blocked. To claim demonstrated contention, add narrowly scoped test-only instrumentation that observes a failed try-lock immediately at the actual acquisition site then proceeds through the equivalent normal lock path, and require that event before callback exit; verify lock/interrupt semantics. Otherwise narrow the claim. Use release/acquire atomics and coherent event sequencing. Callback timeout/cap exhaustion is inconclusive. UNKNOWN vector dispatch still changes load accounting (`interrupts.cpp:245-255,358-372`), hence isolated-VM-only and no zero-side-effects claim. Verify DEBUG_INTERRUPTS behavior, reject unexpected dispatch and preserve single-open, kernel-owned data and module lifetime through team exit. Astra notified; no test executed.

## Astra disposition: native IRQ design review

Accepted Terra finding that removal-entry overlap does not prove lock contention.
Revised proposed test-only hook to record failed try_acquire_spinlock before the
ordinary blocking acquisition, retaining successful try-lock ownership without
double-acquisition. Only exact armed vector/callback/data is instrumented. Require
contention-before-exit ordering; uncontended/deadline rounds are inconclusive.
Accepted explicit atomic publication, DEBUG_INTERRUPTS/count checks and statistics
side effects. Revised design awaits independent disposition; no implementation.

### Terra: refinement of IRQ contention instrumentation

A fixture-only failed try-lock at the actual remover acquisition site, followed by normal acquisition, addresses the entry-only evidence gap when the callback has entered and waits while holding the verified vector lock. `headers/private/kernel/smp.h:233-245` shows the non-debug ordinary acquire already starts with the same atomic try operation. Preserve ownership on try success (do not acquire twice), use ordinary acquisition on failure, release exactly once and restore the original interrupt state. Check actual spinlock-debug build configuration/bookkeeping, match run ID and exact callback/data, publish events with release/acquire semantics, and require no timeout, successful removal and both worker returns. This validates an instrumented schedule, not every schedule or physical IRQ masking. No design-level blocker identified; implementation remains to be reviewed.

Terra accepted revised design: failed try-lock records actual contention; inline
non-debug acquire uses the same initial primitive (smp.h:233–245). No design-level
blocker remains. Implementation must confirm DEBUG_SPINLOCKS/CONTENTION semantics,
release exactly once, match run ID, require no timeout and both workers returned.
Terra must review code before execution; no executable change or new test yet.

### Terra: concrete isolated IRQ hook review

Reviewed `headers/private/kernel/irq_drain_test.h`, `src/system/kernel/irq_drain_test.inc`, and removal hook in `interrupts.cpp` in the isolated validation worktree. No blocking defect identified under stated caller serialization, exclusive ownership, worker-join and module-lifetime preconditions. `headers/os/support/SupportDefs.h:262-309` supplies release/acquire atomic set/get and sequentially consistent compare-exchange/add. `src/system/kernel/smp.cpp:258-279` shows debug try-lock checks and contention accounting; successful try retains the lock, unsuccessful try falls through normal acquire, and existing removal releases once. Record actual debug flags and enforce SMP; fixture must enforce lifetime/team-exit and validate callback timeout and ordering. Verify ordinary builds have no hook exports using fresh separate output or a complete rebuild; toggling a Jam flag alone does not prove old objects were rebuilt. Fixture/client, boot and runtime have not been reviewed or executed.

### Terra: delayed-reader hook-state fix

Reviewed revised `src/system/kernel/irq_drain_test.inc`: release-published active run, acquire reads, monotonically increasing bounded run IDs and static never-reused records avoid rewriting target fields while a delayed unrelated remover can still observe the previous record. Mutable event fields are atomic. No blocking issue identified under serialized setup/finish and joined-worker lifetime. `sInitializedDrainRuns` is non-atomic: serialization must cover all arm callers, opens and team exit; report 32-record exhaustion without wrap or reuse. Fixture code and runtime remain unverified.

### Terra: kernel-resident IRQ fixture design

No inherent blocker identified in moving fixture code/data and callback into the opt-in test kernel. Admission must atomically gate worker ioctls and increment inflight before dropping the common mutex; finish must transition to closing under that mutex to prohibit new/duplicate workers and require both actual returns, inflight zero, exact successful removal and empty snapshot before release. Team exit/close must prohibit admission and retain resources if uncertain. A kernel-resident callback does not preserve the thin device bridge instructions/cookie: verify framework guarantees bridge lifetime through every in-flight call or explicitly retain it. Require kernel-side privilege checks, failed-open rollback, bounded waits and watchdog; do not pass user pointers to internal kernel hooks. No implementation or runtime reviewed.

### Terra: legacy ioctl, cookie and bridge lifetime

For ordinary ioctl/close, `src/system/kernel/fs/fd.cpp:495-523` holds a `FileDescriptorPutter(get_fd(...))` over the call; 282-307 increments descriptor refcount, and 182-197 invokes `fd_free` only when the last reference drops. `vfs.cpp:6353-6359` passes the descriptor cookie through the filesystem; `device_manager/devfs.cpp:1187-1201` calls device Free, UninitDevice and frees its cookie on the free path. `legacy_drivers.cpp:1277-1294,1298-1309` increments and decrements device usage, unloading on final uninitialization if unpublished. `AbstractModuleDevice.cpp:125-129` calls the bridge control hook. This supports ioctl-time retention of the cookie/device and normal unload path. However `fd.cpp:224-235` invokes Close when open_count reaches zero even while ioctl references remain, so Close may execute concurrently and must not free state used by control. Scope excludes arbitrary explicit driver self-unload, forced teardown or unusual unpublish/reload behavior; retain fail-closed state and test team exit. No native test performed.

### Terra: kernel-resident IRQ fixture implementation review

Reviewed `src/system/kernel/irq_drain_fixture.cpp` and private header; no immediate release/use-after-free blocker found under stated single-owner, persistent-kernel-state and bridge lifetime constraints. However remover CPU is sampled at lines 147-149 before a potentially one-second sleep loop, so it does not establish the CPU at lock acquisition. Capture/validate CPU at the interrupt-disabled acquisition hook. The sequential control waits only for callback entry at lines 149-156; require dispatch completion before admitting its removal or explicitly treat contention as control failure. Pass/failure reporting must distinguish timeouts as inconclusive. Setup rejects low vectors fail-closed as intended. Bridge user-copy, static-cookie lifetime, actual worker joins and team-exit behavior remain to be reviewed. No runtime performed.

### Terra: IRQ bridge and client review

The thin bridge uses a kernel-local request/result and a static cookie; Close does not free it. The revised fixture records the remover CPU at the interrupt-disabled hook and gates sequential removal on dispatcher completion. A fail-closed gap remains in `src/tests/system/kernel/irq_drain/client.cpp` `execute_workers`: even when either readiness/affinity check fails, it releases worker go flags; thread creation failure can also release dispatch. Thus one ioctl can still run after per-round affinity preflight fails. Add a cancellation flag checked before any ioctl, join created workers without starting either on failed readiness or creation, and preserve terminal retained state on failure. Sequential mode needs both worker affinity checks before starting its dispatch while still waiting for dispatcher return before removal. External watchdog remains required. No live test performed.

### Terra: IRQ worker cancellation correction

Reviewed revised `client.cpp`: both worker readiness checks precede go; cancellation is published before go and checked before ioctl; failed second thread creation cancels and joins the first. Sequential removal follows dispatcher join and is cancelled on failure. Main skips finish and retains resources after worker failure. Default sequentially consistent atomics support publication ordering. The worker go deadline can expire during a slow sequential dispatch; that leads to not-pass and retention, not a false pass. This resolves the reported readiness gap in source review; native runtime and watchdog remain unverified.

### Terra: x86 disposable IRQ fixture vector threshold

Source supports changing the test-only rejection threshold from <=31 to <16, conditional on the existing allocation, no-lock, handler, UNKNOWN type and CPU assignment checks. `src/system/kernel/arch/x86/arch_debug_console.cpp:172-179` only removes the keyboard handler for IRQ 1. `arch/x86/ioapic.cpp:818-827` reserves each discovered IOAPIC range using its actual global base and maximum redirection entry. `arch/x86/arch_int.cpp:238-240` subtracts `ARCH_INTERRUPT_BASE` (0x20), so allocation indices 16..31 are not x86 IDT exception vectors 16..31. UNKNOWN plus B_NO_ENABLE_COUNTER avoid tested install routing/enabling, and direct software dispatch is not a physical IRQ. Actual guest allocation and controller state remain to be verified; no assertion that first free is 24 or that firmware/untracked routing cannot conflict. Disposable VM and watchdog only.

### Terra: IRQ VM result verifier review

`installation/private/kernel-irq-002/verify-rounds.py` independently checks 11 ordered rounds, CPUs, debug configuration, timeout flags and client/identity status. `run.py:117-120` however marks clean shutdown from QEMU exit code zero after requesting shutdown; it does not check the shutdown SSH result or a QMP shutdown event, so this alone is weaker than proof of guest-initiated clean shutdown. Corroborate using serial shutdown evidence or QMP events, or describe it as QEMU exit zero after shutdown request. Also preserve partial stdout/stderr from `TimeoutExpired` on client timeout; existing `subprocess.run` exception skips writing `client.log`. Python assertions must not be disabled via `-O`. Review was read-only while VM active; result pending.

### Terra: independent review of native IRQ fixture first run

Separately parsed `installation/private/kernel-irq-002/client.log`: 11 rows, vector 24, CPU 0 dispatcher and CPU 1 remover/hook, one callback and zero timeout each; control sequence 1,0,2,3 and ten contention sequences 1,2,3,4. Its SHA-256 matches `round-verification.json` (`80404b9edf8c5672b0bfdbade50080f6f233ba9ffe9821a291f7611ae28da722`). `serial.log:277` contains the instrumented kernel revision; result records successful identity gate and client exit 0. QEMU exited 0 after shutdown request but no QMP event was captured, so clean guest-initiated shutdown remains unverified for this run. This supports the instrumented VM software-dispatch contention behavior for those rounds only; it does not prove physical IRQ masking, team-exit handling, all schedules, or GPU execution. Terra did not rerun VM.

### Terra: independent repeat IRQ evidence review

Separately parsed `kernel-irq-002/repeat-client.log`: 11 expected rows, one control and ten observed contention sequences, correct CPU IDs and debug flags, no callback timeout; SHA-256 matches repeat verification. `repeat-serial.log:277` has the instrumented revision. `repeat-qmp-events.json` records `SHUTDOWN`, guest=true, reason=guest-shutdown; repeat result records QEMU exit 0 and client exit 0. Shutdown SSH status was 255, retained as recorded; QMP supports guest-origin shutdown but does not independently prove every userspace flush. Across two VM boots evidence supports two controls and 20 instrumented software-dispatch contention rounds, not physical IRQ/GPU behavior, team exit or all schedules. Terra did not rerun VM.

## Astra disposition: native IRQ evidence review accepted

Terra independently parsed both eleven-round logs, checked digests and kernel
serial labels, and accepted repeat QMP guest-shutdown/exit0 evidence. SSH255 is
recorded disconnect status; no claim every userspace flush was verified. Scope
remains2 sequential+20 instrumented software-contention rounds, not physical IRQ,
GPU execution, team exit or every schedule. Terra did not rerun the VM.

### Terra: semaphore owner exit/exec reproducer review

`gpu-driver/tests/sem-owner-exit.cpp` uses a blocked-waiter count check before owner exit or exec, then checks waiter and stale-ID operations, and the exec handshake confirms the owner team remains alive. `replacement != old` only proves distinct IDs for that allocation, not same-slot reuse or generation protection. Failure handling should avoid spawning an unresumed thread when an earlier `ok` is false (`lines 73-75`), since waiting for it at line 98 can run until the global alarm; terminate/resume and join as appropriate. Ignore/handle SIGPIPE on failed child communication and close/reap on early errors where feasible; retain host watchdog. No execution or physical GPU conclusion.

### Terra: owner-exit reproducer cleanup correction

Reviewed revised `tests/sem-owner-exit.cpp`: owner handshake now precedes waiter creation, common failure handling kills/reaps the child and stops/joins a created waiter, pipe/fork cleanup is present, SIGPIPE is ignored, and owner action follows proof of blocked waiter. Previous cleanup concern addressed. Blocking I/O still requires the global alarm and host watchdog; normal waitpid EINTR retry would improve resilience. Different replacement ID does not prove same-slot generation behavior. No runtime performed by Terra.

### Terra: owner-exit native VM evidence review

Checked `installation/private/sem-owner-001/test.log` and `result.json`: owner exit and exec both returned B_BAD_SEM_ID to blocked waiter and old-ID info/count/release/delete; new IDs differed. Source SHA-256 matches result; QMP records guest-origin shutdown. Native baseline VM evidence confirms semaphore semantics for these cases, not same-slot reuse or GPU driver/physical hardware behavior. Terra did not rerun the VM. Astra notified that retrace research file still described runtime as pending at review time.

Astra accepts Terra independent sem-owner-001 evidence review: source hash,
two test rows, baseline kernel identity and QMP guest-shutdown corroborate the
native reproduction. RETRACE-LIFETIME.txt updated from pending to actual result.
No same-slot reuse, wrap, driver teardown or physical GPU claim.

### Terra: IRQ fixture close-race design review

The controlled close/callback overlap can be shown with ordered atomic ENTER < CLOSE < EXIT events: callback waits acquire for closeSeen, close records CLOSE before release-publishing closeSeen, and the test requires observed close, no timeout and strict event order. Independent entered/not-exited snapshots alone have a TOCTOU gap. WAIT_ENTER must not hold the fixture mutex while waiting and must release its descriptor reference before close. Callback must not take mutex, sleep or log. Record final state in normal context after releasing fixture mutex; correlate run, complete dispatch, inflight zero and retained handler/vector. Validate Free occurs only after dispatch ioctl returns and actually attempt reopen. Fresh disposable boot and host watchdog; intentional handler retention must not escape it. Test does not establish team-death behavior, hardware IRQ masking, removal, or GPU execution. Implementation review pending.

### Terra: IRQ close-race implementation review

Reviewed isolated `irq_drain_fixture.cpp`, hook and bridge/client changes. Close event is recorded before release-publishing closeSeen; callback observes it before exit; WAIT_ENTER drops fixture mutex, client joins dispatch and attempts reopen, and the static bridge cookie is not freed. No design-level blocker identified. Serial `IRQ_CLOSE_RESULT` should include and require `events.invalid_event == 0`, currently omitted, along with read/retained status, count, timeout, observed close, CPU IDs, inflight zero and strict event sequence. Verify bridge dispatch-return precedes Free. Absence of an Uninit log alone does not prove it cannot occur. Host verifier must combine serial and client evidence; timeout before close is inconclusive. No runtime reviewed.

### Terra: close-race host verifier review

`kernel-irq-close-001/verify-close.py` requires client close/reopen denial, strict serial event order and no duplicates/timeouts, CPU IDs, retained handler and QMP guest-origin shutdown; runner preserves partial client output on timeout. No blocking acceptance gap identified. `IRQ_BRIDGE_DISPATCH_RETURN` is emitted immediately before bridge_control returns, so marker-before-Free directly proves bridge-tail ordering; the separate descriptor-reference source trace supports later function lifetime. QMP guest shutdown does not prove every userspace flush. Python assertions require no `-O`. Runtime result not reviewed yet.

### Terra: close-race native VM evidence review

Independently checked `installation/private/kernel-irq-close-001/`: client and serial hashes match verification JSON; serial records callback enter/close/exit 1/2/3, dispatch CPU 0, close CPU 2, callback count 1, no timeout/duplicate, inflight 0, retained exact handler and read checks B_OK. Marker order is result, bridge dispatch tail, Free, Uninit; client records actual reopen denial and QMP records guest-origin shutdown. This supports one instrumented controlled-close case, not team exit, real IRQ or GPU execution. Revision label `git-d9507b4d-irq-close-candidate` differs from recorded source commit `2d71916f...`: treat it as a test label and use recorded binary/package hashes and build provenance for exact attribution, not label alone. Terra did not rerun VM.

### Terra: proposed vblank semaphore cleanup gate

Blocking design gap: `interrupt_handler_installed == false` after successful `remove_io_interrupt_handler` does not itself prove callback quiescence. The kernel vector no-lock flag can persist or be set by a shared registration; removal takes the lock but such dispatch can skip it. The native contention fixture covered only a snapshot-verified locked vector, not arbitrary production vectors. Deleting/invalidation based solely on this flag can therefore race a live callback; existing area/code teardown already has a related open defect. Establish a production callback-lifetime/drain mechanism, or confine cleanup to proven never-installed/no-asynchronous-producer paths. Initialize private semaphore sentinel before full init failure paths; update shared mirror only while valid; do not overwrite an ID retained after unexpected deletion failure on reopen. `B_BAD_SEM_ID` after owner exit is expected under the OS ID contract; eventual generation wrap remains unproven. Exact implementation and regression tests pending.

### Terra: independent host IRQ removal race reproduction

Reviewed and independently ran `tests/irq-remove-contract-host.py` into `/tmp/terra-irq-contract-independent-001`: extracted kernel functions compiled under ASan with modeled synchronization and CPU infrastructure. Locked case returned after callback exit; deliberately orchestrated no-lock case returned before callback exit and produced the expected ASan heap-use-after-free in handler traversal. The locked attempt marker precedes lock acquisition and alone does not prove contention; separate native fixture captured failed try-lock. This expected-defect host model does not establish that a physical N100 uses a no-lock vector or demonstrate physical GPU behavior.

## Astra disposition: no-lock removal objection accepted
Successful removal does not imply general drain. Withheld proposed semaphore
cleanup; production unchanged. Extracted actual dispatch/removal host harness
reproduces no-lock handler-node UAF under ASan, with locked control. Terra reran
independently. Locked host marker does not prove contention; previous native
fixture covers its locked vectors only. Preliminary permanent locked-registration
latch remains conditional on version handshake, rejection before side effects,
reuse/internal-consumer audit and separate dispatcher-tail/mask handling.
See research/n100-submission/IRQ-TEARDOWN-CONTRACT.txt. No physical reachability claim.

### Terra: isolated dispatcher-tail patch and host matrix

Reviewed candidate moving locked vector unlock after update_int_load while preserving early empty-handler unlock. Independently ran actual extracted dispatcher/remove/free with modeled accounting for DEBUG_INTERRUPTS 0/1 and edge/level: all four baseline cells exit 2 with invalid assignment; all four candidate cells exit 0 with valid assignment and observed failed try-lock. Evidence `/tmp/terra-irq-tail-{0,1}-{edge,level}-001/` and corresponding baseline directories. Forced already-locked schedule only; no conclusion about late/waiting/no-lock delivery, IRQ-disabled latency or physical hardware. No production integration approval.

### Terra: tail candidate native regression evidence

Separately parsed `installation/private/kernel-irq-tail-001/client.log`: 11 expected control/contention rounds and matching SHA-256; serial identifies tail-candidate label; QMP shows guest-origin shutdown. Result records identity gate and exit 0. Build manifest records base commit plus candidate patch hash, instrumented kernel and normal build with zero fixture exports; its `booted:false` is build-stage status, while separate VM result records boot. The native fixture exercises callback-removal regression, not a forced accounting-vs-free tail schedule; that targeted schedule is currently host-model evidence. No late/no-lock/physical proof or production approval.

Astra accepts Terra A11 independent matrix/runtime review. Isolated5ccc8871
reasonable within documented limits; not production approval. VM11rounds cover
callback-removal regression; forced accounting schedule remains host modeled.
Buildmanifest booted:false is build-stage status, later runtime result proves
VM boot. No-lock/late/hardware behavior not established.

Astra accepts Terra physical-memory001 evidence review:30exact rounds/sizes,
PCI/component identities and hashes, app_server and inspected1920x1080 result.
One physical serial owned-memory smoke only. xHCI max_burst_payload/stall/recovery
errors remain noted. No GPU workload, leak freedom or teardown completion claim.

Astra accepts Terra independent physical-memory002 review:30ordered rounds,
exit0, hashes matching first run, PCI/loaded paths, app_server and visible result.
Boot007 is shutdown dialog,008 desktop; capture errors/cache exclude any claim
about first HDMI signal or BIOS visibility. Terra did not operate hardware.

### Terra: two-worker physical memory test design

Both successful returned ranges must be simultaneously live before either free; check address-plus-size overflow and nonoverlap. If ranges overlap or descriptors duplicate, quarantine both and retain/log them: blindly freeing both can double-free or release the wrong backing. On allocation failure hidden partial backing can remain; never guess cleanup. Bounded barriers must cancel/broadcast on error, and in-flight ioctl workers retain all referenced state/fds until return; external watchdog handles a stuck kernel ioctl. Per-worker ownership is test bookkeeping only: `device.cpp:356-367` frees by base through shared device info, not a proven kernel per-fd ownership contract. Preserve primary display lease and artifact checks; physical test would establish only observed returns and virtual-range separation, not physical page separation, leak freedom or GPU execution. Design only.

### Terra: concurrent-client diagnostic source review

Reviewed `physical/memory-concurrent.cpp`: atomic publication, both valid/nonoverlapping range checks before free, ambiguous-result quarantine, static state and joins appear consistent with reviewed design. No obvious blocking UAF/double-free in expected paths. Quarantine loops without a 20-second process alarm; active external observer/reset is mandatory. Start barrier does not establish simultaneous kernel execution; it establishes two clients with simultaneously live allocations if both results complete before free. Returned `size` echoes request rather than independently proving actual mapped extent. When validation times out, short-circuit may omit a completed worker descriptor from logs; after successful joins, log both completed states before quarantine, never read fields during a still-running ioctl. Run self-test and host cancellation/failure-injection controls before physical test; verify primary display lease and loaded identities independently. No runtime reviewed.

### Terra: isolated concurrent diagnostic failure controls

Host harness built from actual `physical/memory-concurrent.cpp`, with mocked OS/ioctls and generated-copy quarantine changed to exit 77. Success: 20 allocations/frees/joins; overlapping results: 2 allocations, no free; one allocation failure: 2 calls, no free; second thread creation failure: no allocations, first thread joined. All passed expected counters. Source digest and logs: `/home/dan/renkuos-terra-vulkan/test-controls/concurrent-host/`. Host model only; no actual kernel, stuck ioctl, exhaustive scheduling or physical validation. Source unchanged.

A16 Terra review: encode/bounds correct; fixed enum underlying type required. Astra accepted and corrected uint8_t, tests2/255 plus mixed indices/offset. Terra independently passed2150 UBSan cases and accepted offline candidate. Native2150pass recorded separately; no hardware VM approval.

A17 Terra independently approved final offline constructor after design/code review and69UBSan cases. Astra accepted added uncached/PAT/boundary coverage and non-reentry/physical exclusivity requirements. Native69pass recorded separately; no hardware VM approval.

A18 Terra accepted final provider and explicit encoder-success guards after independent O2 C++17 UBSan runs:69tree and35adapter cases. Native compile/link-only evidence remains distinct from runtime; no production/publication approval.

A19 Terra accepted resident module/bridge/client source for bounded memory-only physical test. Native compile passed. Runtime not attempted because console failed before installation; no kernel allocation result claimed.

A20 Terra independently accepted exact GGTT packet/header/address/complement and bounds after32O2UBSan cases. Native/hardware unverified; payload is incomplete request until reviewed flush/context/retirement.

A21/A22 Terra found no blocking factual errors; Astra accepted stricter-policy attribution and all-unit/scope/platform DMA constraints clarification. Historical DMAR is archived-log evidence; no live readiness or hardware programming approval.

Terra independent physical evidence disposition: PASS within wired-page kernel
scope. All8source/header hashes and3rebuild binaries verified;installed hashes,
PCIidentity,client0,3cases/42areas/live0/cachedresult and inspected terminal agree.
No60second post-test observation claim: snapshots span about36seconds. Successful
delete returns,emptyledger andold-IDabsence support cleanup,not exhaustive physical
page accounting. No GPU DMA,MMIO or execution proof. Astra accepts these limits.

A23 Terra accepted source-specific retained ACPI lifecycle,parser/snapshot implementation andphysical136byte inventory.96hostASan/UBSan cases andnative96pass. Rawtablechecksum/scopes/RMRR/source/build/installed hashes independentlychecked;translationstateUNKNOWN. Astra accepts scope andlifetime limits.

A24 Terra accepted finalmappedoverflowguard,independentlypassed79helper/3actualmodule mockedscenarios andverifiedphysicalrawsamples/source/build/installedhashes. Rootacceptsinterval-only scope;no controllerwrites,identity/RMRRorGPUexecutionclaim.

Terra independent physical A25 disposition: PASS for three kernel cases,42raw-
firmware exclusion checks,zero live provider areas,cached match andexit0. All12
source/test hashes andinstalledmodule/bridge identities match the finalrebuild.
This covers allocatedpages inthisrun;syntheticcollisions remainhost evidence.

A26 Terra disposition accepted by Astra: PMCSR/header overlap defect corrected
for either traversal order.302 helper and10 actual-function race cases independently
passed ASan/UBSan. v1 gets/retains v0 without second initialization; PCI outside
lock, operation pin spans Begin/End, uncertainty sticky-blocks. Narrow fixture
attempts all retirements before copyout. Final source/native/installed/backup
hashes and six archived physical PCI sessions +20 paired allocations/frees
independently verified. No GT/persistent-power/DMA/execution claim.

A27 Terra design/code review accepted for native build and bounded read-only
physical trial after artifact checks. Explicit UC on same retained owner map,
v2 Reading phase, fixed130044/32-bit bounds, read barriers and final PCI/state
retirement.26 actual-function mocked cases independently passed ASan/UBSan.
Astra accepted correction: invariant mismatch deliberately retains pin rather
than guess-retire; bridge comment now states that. Raw MMIO hangs cannot be
recovered by software alarm. No request write/wakeref/engine/DMA/execution claim.

A27 final disposition: Terra independently accepted matched native/installed
owner29570eb...,bridge79d3dc8...,A26backup3ca113...,26 actual-function host/native
mock cases and physical live/post-memory eight rawzero ACK samples.20 paired
alloc/free passed and display/GART binaries unchanged. Astra accepted limits:
raw read cannot prove request ownership, persistent power, handshake or GPU
execution. Bridge comment corrected to document retained invariant-failure pin.

A28: Terra found missing mandatory posting read on cancellation, readback
excluded from50ms stages, incomplete admitted reports, persisted stack callback
context and non-atomic admission. Astra accepted/corrected all before physical
trial. Independent67 pure+37 actual-owner ASan/UBSan and native cases pass.
Final source/artifacts accepted for one bounded controlled trial. Physical
ACK0->1->0,2 software-recorded writes,9 reads,454us/exit0;postrelease memory/PCI/
ACK/display accepted independently. Exact words derive from reviewed source,
not separate bus trace. No engine execution,global power or firmware authority.

A29 source-only disposition: Terra independently verified84 sharedstate/37
sampler/48actualowner mockedcases. Typeduncertaintycorrected ambiguousfalse
cleanup; everyrawones/clock/bounds fault propagatesUnknown. Close/backing/final
PCI faults preserveoutputcontractandpin. Fixtureonefixedv4call/nooffsets; client
checks0->1->0/fallbackclear/rawnonones. Nativeartifact/physicalreview stillpending
restoredaccess. Hostcommit isnotdeploymentapproval.
