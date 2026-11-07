# Hardware and validation evidence

Physical display: historical build 057 HDMI at tested 1920x1080 RGB8.
Physical GPU execution: NONE.
Native compilation/VM: build 072 passed bundled fixtures, not real Intel device callbacks. Terra R1 proves those fixtures missed a lock deadlock; do not use that result as evidence of hardware readiness.
Host: corrected production helper/query/unmap threaded fixture passes 100 rounds after R1 fix. Independent validation pending.
Recovery: current image hashes, bootability, management and NanoKVM connectivity must be checked before controlled physical tests.

## Terra evidence classification (2026-11-07)

HOST-SIDE: R1 old snapshot reaches blocking semaphore path; corrected production
helper/query completes; corrected actual-helper threaded fixture passes 100
ASAN/UBSAN rounds; old snapshot negative control fails within bounded gate.
Source/OS/backend registry services are modeled as documented in the harness.
NATIVE COMPILATION / VM: no independent Terra run in this assignment.
PHYSICAL GPU EXECUTION: none. Physical memory validation remains pending.
SOURCE REVIEW: pinned Linux research and RenkuOS module/driver lifecycle inspection.
Specifications in GPU_FIRST_EXECUTION_PLAN.md are not executed test results.

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

### Live access probe, 2026-11-07

Read-only SSH probe through existing strict-host-key renku-laptop config failed
with exit 255: No route to host. No remote command executed, deployment, reboot,
recovery test or GPU workload. Historical target address is not proof of current
connectivity. Physical validation remains blocked on live target/recovery access.

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

## Terra independent native VM RAM clone probes

NATIVE VM / RAM surrogate: same-team two-clone and forked cross-team clone probes compiled with -Wall -Wextra -Werror and both passed after original area deletion on Haiku hrev60072+518 x86_64. Disposable QEMU overlay, no passthrough; clean guest shutdown; canonical host-config/VM/build locks held throughout. Sources SHA256 d2f7ab15f7f07d3fcec21cfe9c5c2e0d270ca7930fa533e74cfd454e9301c7f3 and 7b92946317687bd1f255498e3ec4f51878bafabcfe6c46b8c1b3bf09daa821a7. Evidence /home/dan/renkuos-terra-n100/research/terra-n100/native-owner/vm-run-001/{identity.txt,native.log,result.json,serial.log}.

These are userland RAM areas, not module-owned kernel-MMIO areas or actual accelerant clones; no GPU execution, PCI passthrough or physical test. Compiler/binary hashes were not captured for this exploratory run; capture them in the later integrated validation.

NATIVE VM negative control: a separate child team attempted to clone a RAM area without B_CLONEABLE_AREA; clone was rejected as expected. Disposable QEMU VM hrev60072+518 shut down cleanly. Source SHA256 97b3042399b9970526f43eefb9ce75433afd9db04cf7ad685ba4e8b6d585ddcc; logs and result at /home/dan/renkuos-terra-n100/research/terra-n100/native-owner/vm-run-002/. Native OS RAM areas only; no GPU MMIO or hardware execution.

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

Terra review of Astra native kernel probe first attempt build-063: kernel-probe.log reports actual owner module lookup status 0, impossible PCI rejection B_NOT_SUPPORTED, unchanged output and user clone of RAM kernel area surviving test-device close. serial.log independently records test driver load and AGP manager initialization reporting zero AGP devices. Binary hashes in probe-hashes.json. Client test passed, but later collector failed decoding binary syslog; VM was stopped by QMP cleanup, so no clean shutdown/finalizer success is claimed. Repeat build-064 in progress by Astra. This is a native VM module/admission + RAM surrogate result, not real GPU/MMIO/physical execution.

Terra independent review build-064: probe-result.json exit 0 and kernel-probe.log report real owner module load, impossible BDF rejection with unchanged outputs, and kernel RAM clone surviving original close; serial records test driver load and AGP reporting zero devices; native.log records clean VM shutdown. Independently recomputed and matched all three artifact hashes in probe-hashes.json. This is native VM module-manager plus RAM clone evidence; no actual N100, GPU MMIO, IRQ or execution. Earlier build-063 client passed but had collector failure and forced cleanup.

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

### Native089 result and scoped0088 acceptance

Compilation, native component fixtures and finalizer pass; VM067 shut down cleanly.
Candidate source byte-matches exported bundle. Driver SHA-256: 7eb88dcdcdd1fa5455b15628853c7f18268e2878afb5135586e32bf84b5836e2.
Evidence: installation/private/gpu-hdmi-build-067/native.log, finalize.log,
validation/result.json. Terra independently reviewed0088 source/host behavior.
Added full-driver retained-handler reinit rejection check passes before allocations.
No actual IRQ dispatch, new kernel probe, candidate driver installation or physical
GPU execution in this run. Full IRQ safety and normal semaphore cleanup remain open.
Next: native kernel dispatch/removal probe design and sem owner-exit lifetime contract.

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

## IRQ removal host contract checkpoint
Simulation/host: actual extracted kernel functions with modeled OS infrastructure;
locked control and expected no-lock ASan defect observed. Host compilation passed.
Native compilation: none new. VM: none new. Physical GPU execution: none.
This does not establish production N100 vector mode or physical bug occurrence.
Evidence: tests/evidence/irq-remove-contract-002/. Known HDMI baseline unchanged.

A11: host modeled accounting matrix4before failures/4after passes; normal and
instrumented kernel cross-compilation passed. Native disposable VM11software
IRQ dispatch rounds passed; QMP guest shutdown. No physical GPU/IRQ test.
Native fixture is regression coverage, not a forced accounting-tail overlap.

## Physical N100 memory001 — actual hardware
Compiled native089 bundle on N100 and finalized source/artifacts/fixture results.
Backed up components, installed matched driver/accelerant/GART/owner on RenkuTest
USB, rebooted, verified PCI8086:46d1, loaded paths and postboot hashes. Thirty
owned NON_RESERVED allocate/free cycles passed,4KiB/8KiB/64KiB, app_server127
remained running. NanoKVM1920x1080 desktop and visible results captured. Wi-Fi
rejoined automatically. No new simulation/VM here. No GPU commands executed.
Single boot, no fault injection, no leak-count/concurrency proof. Syslog also
contains an xHCI slot2 endpoint1 stall; no globally error-free boot claim.
Evidence tests/evidence/physical-memory-001 and private gpu-memory-physical-001.

### Terra independent review: physical owned-memory smoke

Reviewed `installation/private/gpu-memory-physical-001/`: memory-test.log records all 30 serial owned NON_RESERVED allocation/free cycles (4 KiB, 8 KiB, 64 KiB); PCI 8086:46d1, loaded driver/owner/GART paths, target artifact hashes and app_server team 127 before/after are recorded. Inspected after-test.jpg: 1920x1080 desktop shows final pass. This supports the bounded physical allocation/free smoke, not GPU command execution, concurrent safety, teardown correctness or leak freedom. Syslog also contains xHCI ConfigureEndpoint invalid max_burst_payload and slot 2 stall/recovery; no claim of globally error-free operation. Terra did not perform the hardware test.

### Terra independent review: physical warm-reboot repeat

`installation/private/gpu-memory-physical-002/` records 30 ordered owned allocation/free rounds and MEMORY_EXIT=0. Repeat component hash list matches first physical run byte-for-byte; PCI 46d1, loaded paths, accelerant and app_server team 127 before/after are recorded. Inspected visible-repeat-result.jpg with full pass. Inspected boot-007.jpg (shutdown dialog) and boot-008.jpg (desktop); frames.json reports response times 28.476 and 29.735 seconds after key with earlier API capture errors. These observations do not establish first physical HDMI signal or BIOS/bootloader visibility because capture timing/caching is uncertain. No GPU commands, concurrency stress or leak proof. Terra did not operate hardware.

Physical-memory002: visible NanoKVM reboot/preflight/test;30additional owned
serial cycles passed exit0, app_server unchanged. Same6hashes and loadedpaths
verified, fresh USB boot. No new compiled/simulated/VM work; no GPU workload.
60cycles across2boots total. Early HDMI first-signal timing remains unproven: API
errors then shutdown/desktop frames, no BIOS/bootloader captured.

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

A26 — physical N1008086:46d1 rev00, kernel hrev60072+518, RenkuTest USB
Implemented: v1 read-only owner admission/retirement, retained v0 singleton.
Native compilation: candidate owner/bridge/client linked, --no-undefined passed.
Host simulation:302 PCI cases +10 actual-function thread cases ASan/UBSan passed.
Native simulation: same302+10 passed using mocked config on physical CPU.
VM testing: none for A26.
Physical hardware: software reboot/display and six archived real PCI sessions
passed, command0x407 BAR0xbf000000 PMcap0xd0 PMCSR0. Duplicate Busy/stale Invalid.
Physical memory regression:10 paired rounds20alloc/free passed on Intel device;
source matched existing fixture; display/GART hashes unchanged. Terra verified.
Physical GPU execution: NONE. No GT register access, forcewake or command packet.
Evidence: tests/evidence/physical-pci-owner-001; full raw inventory/syslogs/backup
private at installation/private/gpu-pci-owner-001.
Remaining: Phase1 exhaustive lifetime gates, GT power/mapping/order/reset, DMA
visibility/address/translation, context/ring and completion/recovery.

A27 physical N1008086:46d1 rev00, kernel hrev60072+518, RenkuTest USB
Implemented: explicit UC owner map plus pinned synchronous raw GT ACK v2.
Native compiled: owner/bridge/client --no-undefined passed.
Simulated:26 actual-function host ASan/UBSan and native RAM/PCI mocks pass;
12 prior GART host cases+2 controls pass. VM: none for A27.
Physical hardware: eight raw32-bit samples at130044 all0; bit0/bit15 clear
only at sampled instants. Module/observation/client statuses0.10 paired GART
rounds20 alloc-free and four PCI session cycles passed. HDMI and postboot
app_server127 preserved; old graphics/accelerant/GART hashes unchanged.
Physical GPU execution: NONE. No GT stores, forcewake or command submission.
Terra independently verified source/artifacts/backups/results. Evidence:
tests/evidence/physical-gt-ack-001. Current owner29570eb...;backup3ca113... .
Next blocker: bit authority/power/order and failure cleanup for bounded
forcewake handshake; no inference of ownership or persistent D0 from ACK0.

A28 — physical N1008086:46d1 rev00, kernel hrev60072+518, RenkuTest USB
Implemented: primary-only forcewake transaction with retained owner v3.
Compiled: native owner/bridge/client --no-undefined pass.
Simulated:67 pure+37 actual-owner cases host ASan/UBSan and native mocks pass.
VM: none for A28. Native fixture first link error corrected;private log preserved.
Physical: ONE handshake ACK0->1->0,9reads,2writes,454us,all statuses0,Released.
Postrelease:4 ACK samples0,two PCI sessions and10paired rounds20alloc-free pass;
HDMI/postboot app_server127 retained,display/GART hashes unchanged.
Terra independently verified source/build/installed/backup/result evidence.
Physical GPU command execution: NONE. No DMA/context/ring/render workload.
Scope excludesfallback/repeated reliability/firmware-SMM/global power authority.
Next blocker:engine/context/reset and DMA/cache/translation/completion contracts.
Evidence:tests/evidence/physical-forcewake-001;raw/privategpu-forcewake-001.

A29 pendinghardware: no nativebuild andno physicalengineinventory result.
Implementedcandidate84/37/48hostsimulations pass; source/fixture reviewed.
Transfer failedbeforebuild. A28olderphysicalhandshake remainsverifiedhistorical
evidence; currentlaptopdoesnotrespondreliablytoSSH/NanoKVMHID. Freshcaptured
video remainsoldconsole;software rebootdidnotrestoreaccess. Physicalreset
requested. NoA29driverinstallation,engine/reset/ring/DMA/command stores.
