# ACPI producer closure and worker cessation

Objective `5b5f2f125365d07067a3`, SOL generation 92. Host-only audit/design;
implementation readiness for independent verification, not milestone acceptance.
Parent project commit: `80be0e74e102b701a164bf37747fc5100045858c`.
Source revision throughout: `4799bb5a188c4bb6bbef967287640cc21a061a0d`.
Exact blob/SHA-256 identities and copied inspected files are in
`tests/evidence/acpi-dpc-closure-092/sources.json`. Paths below are relative to
that source tree; ACPICA paths are beneath the ACPI bus manager's acpica/components.
No production edits, compilation, fixture, runtime, remote or hardware operation.

## Source-proven behavior and counterexamples

BusManager.cpp:208–298 creates the DPC queue before ACPICA initialization. Failure
of InitializeSubsystem jumps directly to queue deletion; InitializeTables,
LoadTables, EnableSubsystem and InitializeObjects failures call AcpiTerminate
first. Normal UNINIT also terminates before deletion. Termination only logs
errors; deletion's result is ignored and the global handle is cleared regardless.
Module.cpp:33–38 publishes image dependencies including DPC, PCI and device-manager.
Those dependency pointers and the callback image must survive every uncertain path.

ACPICAHaiku.cpp:860–881 reads gDPC and gDPCHandle without producer custody and
passes the raw function/context to queue_dpc. AcpiOsWaitEventsComplete:1405–1408
returns immediately; AcpiOsTerminate:219–223 always returns AE_OK. Neither proves
cessation. headers/os/drivers/dpc.h declares only create/delete/enqueue status
functions: it documents no lifetime lease, fence, cancellation or join guarantee.

In generic/dpc/dpc.c, queue_dpc serializes admission with the queue spinlock:
thread < 0 rejects; otherwise a full queue rejects; otherwise slot publication,
tail movement and count increment precede unlocking and release_sem_etc. Thus
an enqueue can publish ownership, then return a wakeup error. evmisc.c:187–192
frees notify Info on any AcpiOsExecute failure: a later callback can use freed
Info. This is an ownership ambiguity, not a safe retry condition.

delete_dpc_queue sets thread = -1 under that same spinlock: this is the existing
queue-admission closure linearization, NOT handle lifetime closure. A producer
can already hold a raw handle but not yet acquire its lock. A producer can also
be between slot publication and semaphore release while deletion destroys that
semaphore and frees its queue. No in-flight producer count covers either gap.
Delete ignores delete_sem and wait_for_thread results, frees regardless and
returns B_OK. Therefore B_OK does not establish successful join.

Worker normal dequeue decrements count under lock before executing outside it:
count == 0 does not imply no executing callback. After acquire_sem stops returning
B_OK it drains count/head without locking, relying on producers having ceased.
Closure excludes subsequent accepted insertions but not producer accesses. The
post-decrement drain leaves count negative on exit; it cannot be a zero-count
certificate. Callback-initiated delete attempts self-join then frees its own live
queue if wait returns an error. Duplicate deletion has no single-owner protection.

AcpiTerminate (utilities/utxface.c:67–87) calls SubsystemShutdown, destroys mutexes,
then returns OsTerminate status. utinit.c:310–349 sets Shutdown, calls EvTerminate,
then frees interfaces, namespace, tables, globals and caches. evmisc.c:263–340
logs event disable/removal failures and continues, including GPE handler deletion.
Consequently even AE_OK can follow failed producer disable and destructive
cleanup. Draining AFTER AcpiTerminate is too late to protect callback referents.

Reachable producers include evgpe.c:937–938 (interrupt-dispatched GPE method),
evgpe.c:613–621 (worker requeues enable-GPE; failure invokes it inline),
evmisc.c:187–192 (notify state), and EmbeddedController.cpp:655–681 (interrupt
handler queues EC query and rolls pending count back on failure). Notify dispatch
uses namespace and handler pointers (evmisc.c:176–180, 224 onward). Rejecting a
requeue alone does not prevent its inline fallback or GPE reenable. evxface.c calls
WaitEventsComplete during handler removal; its present no-op supplies no fence.
Textual search evidence is retained; computed/third-party producers are not proved
absent. Queue closure alone cannot certify any of these referents safe to free.

## Required future production contract (not present in this source)

Use one stable, image-owned ACPI admission control block, never the queue itself
as the initial lifetime lock. Initialize it before any possible producer, retain
it through quarantine, and use an IRQ-safe short spinlock. It stores generation,
Open/Closing/Closed/Quarantined phase, handle, producer count and callback custody.
AcpiOsExecute checks phase and acquires a producer lease while copying handle and
DPC dependency under this lock; release only after the complete queue call,
including wakeup. No raw global load before acquiring the lease. Close changes
Open to Closing under this same lock (ACPI producer closure linearization).
Later producers return rejection without touching queue/dependencies; earlier
leases keep both alive. A separate executor waits for producer count zero outside
all spinlocks, Intel sLock and sModulesLock. No counter overflow or generation
wrap may admit a lease; reserve bookkeeping before publication.

Correct DPC internals without altering dpc_module_info layout or unrelated module
ABI: single deleting owner; closure serialized with insertion; explicit queued
and executing counts; decrement executing only after callback returns; drain
under the queue lock, invoke callbacks unlocked; reject self-delete before any
mutation; check semaphore deletion and successful worker join plus exit status
before free. Join must identify the original worker incarnation, never retry a
recycled numeric ID. Failed/ambiguous join retains the queue, wakeup provenance,
worker identity, module image, dependencies and callback backing. Do not replay
delete or enqueue. Caller-side producer leases are still required for raw-handle
lifetime; the queue lock cannot solve access after free for arbitrary clients.

Enqueue must distinguish publication from wakeup: prepublication errors transfer
no callback ownership; after publication, it must report accepted (B_OK), retain
the callback and record any wakeup failure as sticky queue quarantine. It cannot
return a rejection that permits the caller to free context after publication.
Deletion must surface sticky uncertainty and retain ownership. The wakeup/join
implementation and all affected DPC callers require production review before
this behavioral correction; no new public status convention or unrelated ABI
change is proposed. A truthful delete error is necessary but not alone sufficient.

Track accepted callbacks Q and executing callbacks X independently, including the
callback-to-callback gap: enqueue reserves Q before returning acceptance;
dequeue atomically moves Q to X; callback return releases X. Prepublication
rejection releases only untransferred bookkeeping. Retain callback context and
its transitive references through X release. Callback requeue after Closing is
rejected, never retried; the executing parent remains counted through any inline
fallback. Worker-requested shutdown returns a deferred/quarantined result to an
external lifecycle executor and never waits for itself or drops its lease.

Required order: reserve manager Quiescing/image lease; prohibit new client entry;
close ACPI admission; stop and synchronize all external producers; wait producer
leases; close queue admission; drain/join callbacks while their namespace,
handlers, EC cookies and dependencies remain valid; only then run destructive
ACPICA termination and clear the handle after a truthful completion certificate.
Closing admission is not external producer synchronization. GPE inline reenable
must be inhibited during Closing and synchronized IRQ handlers must remain unable
to restart delivery. Required future split of ACPICA event stop/synchronization
from destructive cleanup must propagate every failure, unlike current void
EvTerminate. Do not invoke existing AcpiTerminate as the first stop primitive.
Without that split and an IRQ/client lifetime proof, retain Quiescing/quarantine
and do not invoke destructive termination at all. This is a prerequisite design,
not a claim that shutdown is currently enforceable.

Implement WaitEventsComplete as a real callback fence for its audited callers,
with explicit self-worker handling; never silently return success on a self-wait.
Its void API cannot express failure, so private lifecycle disposition must record
quarantine and prevent callers from destroying referents. Reentrant handler
removal and normal live-queue fence semantics require separate caller review;
a FIFO marker alone misses children queued behind it. No stop/drain under any
module-manager or Intel lock; reserve/commit alone use sModulesLock.

## Interleavings and failed initialization

| Case | Required ownership and disposition |
| --- | --- |
| Producer enters before close | Lease holds handle/dependency through wakeup; closer waits; accepted Q remains owned. |
| Producer arrives after close | Reject without loading handle; caller retains untransferred context. |
| Published slot, wakeup fails | Accepted ownership remains; sticky quarantine, no caller free or retry. |
| Callback requeues during close | Reject child; parent X covers inline fallback; inhibit GPE reenable before producer-stop certificate. |
| Worker requests drain/delete | Reject/defer without mutation or self-wait; external executor owns transition. |
| Event-stop fails or outcome uncertain | No destructive terminate, no dependency/handle clearing; retain all referents and quarantine. |
| Delete semaphore/join fails | Retain handle and all callback backing; no repeated delete of uncertain IDs. |
| Termination fails after proven cessation | Keep handle record, dependencies and remaining backing; record destructive progress, no terminate replay. |
| ACPI disabled before queue creation | No queue custody; release only independently proven unacquired slots. |
| Queue creation fails | Current create frees allocation on allocation/semaphore/spawn failures; no published queue. resume_thread is unchecked: apparent success requires future worker-start proof. |
| InitializeSubsystem fails | Existing err_dpc skips terminate; partial ACPICA progress is not proved absent. Apply stage ledger and same closure; uncertain partial globals remain retained. |
| InitializeTables/LoadTables fails | Existing err_acpi is destructive; future path stops/drains first, retains partial namespace/table state until cessation. |
| EnableSubsystem fails | Interrupt-controller setup has occurred; partial event enable/IRQ state requires stop and synchronization, not queue-only unwind. |
| InitializeObjects fails | ECDT probe has occurred; EC callbacks/cookies require transitive retention and synchronization. |
| Debug handler/runtime-GPE update failure | Existing code logs and returns success; stage ledger must record partial registration, not infer no producer from INIT success. |

A per-stage monotonic ledger records attempted versus completed construction,
registration, stop, join and destruction. Failure never rewinds an uncertain stage
to absent. Do not infer a clean INIT failure from the label chosen by a goto.

## Link to accepted image custody and bounded verification

ACPI-SPLIT-PHASE-ENTRY.md at the parent specifies image-wide Quiescing before
Releasing and per-slot release tickets. Feed that transition a private generation-
bound cessation certificate: admission closed, zero producer leases, Q=X=0,
original worker successfully joined, all producer domains synchronized, no sticky
uncertainty, termination outcome recorded. Manager commit validates image identity
and transition generation under sModulesLock; only then can Releasing reserve
slot releases. No child dependency put is performed during cessation, nor may
an uncertain child consume a sibling's retained slot. Any missing fact yields
Quarantined and retains the image-wide leases and published dependency slots.
Successful DPC cessation is one conjunct, never image-wide permission by itself.

IRQ removal synchronization (ACPICAHaiku.cpp:831–839 delegates to kernel), notify
handler lifetimes, direct ACPI callers/escaped function pointers, EC/region hooks,
SCI/global-lock handlers and firmware/hardware stop semantics remain outside this
bounded proof. evsci.c is pinned to enable independent continuation, not certified.
Unsupported admission, resident v0 display entry and callbacks outside Intel sLock
remain unchanged; no GPU execution or safe DMA retirement is established.

TERRA must reproduce every source hash/blob and search, inspect the actual source,
challenge publication-before-error, raw-handle-before-lock, callback requeue and
inline fallback, self-join, partial INIT and masked termination failures. Verify
candidate differs from the published parent only by this document and protected
HEAD/index/dirty tracked diff remain unchanged. No substitute fixture or repeated
retention matrix: this audit's checks are source identities and interleaving
arguments. Future implementation must test actual production helpers, including
forced wakeup/join failure and simultaneous close/enqueue, before runtime use.
