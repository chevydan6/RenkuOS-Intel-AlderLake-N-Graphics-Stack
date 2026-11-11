# Split-phase module-manager entry contract

Objective `1e6cdd1cd3f1b160472a`; SOL generation 87 (corrects verification-86). Design only, pending
independent TERRA verification and ASTRA acceptance. This completes the entry
contract left open by ACPI-MODULE-RETENTION.md; it enables no lifecycle seam.

## Identity and caller coverage

Evidence is `tests/evidence/acpi-split-entry-085/analysis.json`; preserved generation
83 sources.json pins seven files, blob IDs and SHA-256 at source revision
`4799bb5a188c4bb6bbef967287640cc21a061a0d`. Generation 85 reproduces its
three searches verbatim and compares every pinned byte against Git. All line
numbers below refer to that source revision, not the project documentation HEAD.

Whole pinned src/headers textual search finds only kernel module.cpp:2217
(get_module) and :2270 (put_module) directly invoking the kernel's static
init_module/uninit_module definitions (:714/:777). USB SCSI definitions,
network-test module manager and fs_shell matches are separate implementations;
TRACE strings, comments and declarations are not calls. No exported pointer to
these static helpers was identified by the additional symbol-name search.
Search is textual: preprocessor-generated names, computed module names,
third-party modules and transitive callback reachability remain unresolved.

ACPI.h:16 names bus_managers/acpi/v1. Module.cpp:33–38 declares shared
image dependencies device-manager, PCI and DPC. Its acpi_module_std_ops:267–284
acquires/releases the core ACPI module from the root wrapper. BusManager.cpp:229–238
gets/puts SMBIOS during core INIT. Automatic core acquisitions include Hyper-V,
PCH I2C and display_adapter dependency declarations; explicit acquisitions include
ECAMPCIControllerACPI and radeon_hd. Full search output is preserved, rather than
claiming this shortlist exhausts computed or indirect acquisitions. Future source
changes must rerun coverage, including every callback reached from these paths.

## Exact proposed internal API and ownership

No change to module_info, std_ops varargs, public get_module/put_module signatures
or unrelated module ABI. Add private module.cpp types and functions:

```
enum LifecycleOp { Init, Uninit };
enum Disposition { Ready, CleanFailure, Closed, Quarantined };
struct LifecycleResult { status_t status; Disposition disposition; };
enum ReleaseDisposition { NotConsumed, ConsumedComplete, ConsumedRetained };
struct ReferenceResult {
    status_t status;
    ReleaseDisposition disposition;
    CustodyId retainedCustody; // mandatory for ConsumedRetained
};
status_t reserve_slot_release_locked(ImageLeaseSet&, Slot&, ReleaseTicket&);
ReferenceResult execute_slot_release_unlocked(ReleaseTicket&, EntryContext&);
void commit_slot_release_locked(ReleaseTicket&, ReferenceResult);
status_t acquire_module_entry(const char*, module_info**, EntryContext*);
ReferenceResult release_module_entry(const char*, EntryContext*);
status_t reserve_lifecycle_locked(module*, LifecycleOp, EntryContext&, Transition&);
LifecycleResult execute_lifecycle_unlocked(Transition&);
void commit_lifecycle_locked(Transition&, LifecycleResult);
```

EntryContext and Transition are manager-owned, never public capabilities. Context
contains a synchronous parent chain and an active target set; Transition contains
a unique checked generation, target metadata pin, image reference, operation,
callback identity, dependency ledger and original request ownership. Allocate all
needed bookkeeping before reservation; allocation failure changes no state. Store
active context per current thread in a manager-private map protected by sModulesLock;
install/remove with stack scope, never retain numeric thread IDs after return.
No asynchronous dispatch or live-thread incarnation token is implied by this
synchronous design. Future asynchronous broker work still needs that identity proof.
A child public get/put made from an unlocked std_ops inherits the synchronous
context only on that thread. Other threads start independent contexts and still
see reserved-state rejection. A forged/private std_ops invocation has no transition
and cannot enter the future ACPI lifecycle dispatcher.

Use actual recursive_lock_get_recursion(&sModulesLock), declared lock.h:134,
implemented lock.cpp:52–58. It returns -1 when the current thread is not owner,
and positive owned depth (lock :90 onward increments from zero). Public entry
samples BEFORE acquiring: owned depth >=1 means B_BUSY before search, reference
consumption or state changes. After acquiring, require exactly depth 1. Before
callback/wait, release that one owned acquisition and require -1. Never loop
unlocking an ancestor's recursive acquisitions. Existing recursive callers must
be converted to the unlocked executor, not grandfathered through a depth check.
Private locked reserve/commit helpers never call public get/put or std_ops.

Legitimate dependency recursion is executor nesting with zero owned lock depth:
A INIT -> get dependency B -> B INIT is allowed if B is outside the active target
chain. A -> B -> A, same-target get during INIT/UNINIT, and competing transition
requests return B_BUSY without waiting. Unrelated ready-module get/put is allowed.
An unrelated last put may execute its own unlocked transition; cycles/conflicts
fail busy, never synchronously wait for another transition. Public put B_OK remains
a reference-accounting result; only private ReferenceResult conveys teardown.

## Reserve, unlock, callback, commit

This requires refactoring the common manager executor, not dropping a lock inside
ACPI std_ops. Unconverted nested callers fail closed. Under one lock acquisition,
resolve the target and hold its existing load reference (take an additional image
reference only if the request does not already own one); pin module metadata
against hash removal and image detachment. Check quarantine/transition BEFORE the
positive ref-count shortcut. Separate external refs, permanent B_KEEP_LOADED ref
and transition/quarantine pins; a public put cannot consume internal custody.
Reserve target and generation, then unlock. For INIT resolve dependencies before
std_ops, by child entries of the same executor; append only successful acquisitions
to a rollback ledger. Failure unwinds only acquired records in reverse order, through the slot protocol
below, never the unacquired suffix. ReferenceResult is a tagged accounting outcome,
not inference from status: no ambiguous consumed boolean or public put status may
authorize another release. ConsumedRetained identifies manager-owned child custody;
NotConsumed leaves the exact original reference in the parent ledger.

Dependency pointers live in the image, shared by its exported modules. Add an
image-level dependency lease set: each initialized or transitioning module owns
one lease; first lease builds private records and publishes all info slots under
sModulesLock only when acquisition is complete. Same-context sibling (root -> core
ACPI) reuses the completed set; it must not reacquire or overwrite its globals.
Other contexts encountering Building/Releasing/Quarantined return B_BUSY. Building
cycles fail before callback. Protect leases/state with sModulesLock; child get/put
runs unlocked. An intermediate lease drop neither releases children nor clears
slots. The final lease converts atomically into an image teardown reservation;
retain parent image/metadata until all slot records and child custody are settled.

### Per-slot release and callback-access protocol

This supersedes generation 85's unsafe instruction to retain all shared pointers
on a partial release. A parent image pin does not back any separately loaded child.
Each slot record contains child identity/generation, pointer, slot address, exact
owned-reference token, acquisition order and state: Empty, Held, Releasing,
Released, or Retained. Names alone cannot identify an already consumed reference.
Preallocate records/tickets before reservation; no fallible allocation is allowed
between clearing a slot and recording custody. The release ticket pins child
metadata AND its executable image before the owned reference can be consumed.
It is an internal pin, not an extra external reference preventing child UNINIT.
Every child image-unload/hash-removal path must respect it.

Before clearing ANY published slot, close admission image-wide (including all
exported siblings), finish parent UNINIT or failed-INIT stage cleanup while ALL
acquired slots remain backed, and prove no executing or future parent callback
can touch any dependency. This covers DPC, IRQ, notify, ACPICA workers, exported
entry calls, cached pointers, and callbacks from child teardown into the parent.
Admission zero alone is insufficient: unregister sources, drain/join work and
account for outstanding parent refs and escaped pointers. All valid sibling
leases must be gone; an active sibling prevents final teardown. Gate rejects
same-context sibling reacquisition too once Releasing begins. Cleanup callbacks
may use Held slots while the owner is in Quiescing, but no callback is permitted
after the transition to Releasing. Quarantined images stay closed. Existing raw
global reads cannot enforce this by themselves: future private ACPI dispatch,
entry gates and transitive callback audit are prerequisites. If any source cannot
be gated or quiescence cannot be proven, KEEP every acquired slot/ref and all
callback/resource backing, quarantine, and do not start dependency release.

For each acquired child in reverse order, execute exactly this transaction:

1. Under sModulesLock, validate final image reservation/generation and callback
   cessation proof. Reserve the child release ticket and pin child metadata/image.
   On failure leave the slot/reference Held, quarantine, and stop the sequence.
2. With admission still closed, set the public info slot to NULL and mark its
   record Releasing under the lock BEFORE consuming its owned child reference.
   Keep the pointer only in the inaccessible ticket/ledger. No callback may read
   the cleared slot or a saved copy. Unlock for the common child release executor.
3. Reacquire the lock and validate ticket/generation. NotConsumed changes the
   record to Retained with its original reference and ticket pins, slot still
   NULL. ConsumedComplete records consumption/Released and discards its private
   pointer before dropping ticket pins. ConsumedRetained transfers pins and all
   uncertain child resources to a manager quarantine record BEFORE relinquishing
   ticket custody; record the child CustodyId, consumption, and no callable pointer.
   The child's own remaining dependencies stay backed by its own custody protocol.
4. Only ConsumedComplete permits the next slot. Either other outcome quarantines
   the parent, retains its image, the current record/custody and ALL not-yet-released
   Held slots/references, and stops. Never repeat a consumed put, restore a cleared
   slot, reacquire a released child, or resume automatically. Released prefix records
   remain pointer-free and own no child reference; quarantine does not resurrect them.

Child release atomically records reference consumption and either completed
teardown or retained child custody under sModulesLock; there is no consumed-but-
unowned interval. If callback result, generation or accounting is uncertain, retain
the preexisting ticket pins and complete child transition/resource custody, mark
the ticket unresolved and quarantine the parent; do not guess a disposition or
retry. This requires converting the common child executor and every image teardown
path, not merely wrapping legacy put_module. Unknown child callback behavior is
not completion proof. Public ABI remains unchanged; private results carry custody.
After every slot is Released/Empty and no other image lease/ref/pin remains, the
parent reservation may commit Closed/CleanFailure and detach its image. A parent
with a retained child CustodyId stays pinned even if that child's slots are NULL.

### Initialization unwind uses the same custody protocol

During dependency Building, get into private ledger storage, never directly into
the shared info slots (unlike current module.cpp:695–697). Acquired records are Held;
unacquired suffix records are Empty and never put. No parent callback may run until
the complete set is published. A failed acquisition unwinds the acquired prefix by
the above ticket protocol; slots were already NULL, but child teardown callback
reentry still meets the Building/Releasing admission gate. Uncertainty retains the
remaining prefix and parent; no clean initialization failure is reported.

After publication and entry into parent INIT, failure must first perform stage-aware
cleanup with dependencies Held. In BusManager.cpp:288–299, termination errors or
unproven DPC deletion/join cannot justify any child release: retain gDPCHandle,
callbacks and all dependencies, and quarantine. Only proven callback cessation
allows clearing slots and reverse releases. If a same-image sibling still owns a
lease, drop only the failing module's lease after its own cleanup proof; the shared
set remains complete/backed for that sibling, with no slot clearing. If cleanup is
uncertain, retain that lease and quarantine admission for the whole shared image;
existing users/backing remain counted, and no teardown starts. A failed nested
root/core operation therefore cannot destroy dependencies still used by a sibling.

For UNINIT, consume a valid last external reference under lock and replace its
custody with the transition pin atomically (permanent ACPI B_KEEP_LOADED normally
prevents reaching this point). Close admission under broker C while locked only
for bounded state changes; never call broker callbacks there. Reject same-thread
admitted drain before consuming the reference, using broker-owned admission state;
if that evidence is unavailable return B_BUSY. Unlock before admission drain,
termination, DPC joins or dependency release. No C, TABLES or Intel sLock spans
these operations. Broker completion decrements its count without the module lock;
ordinary reference put may follow. Worker must never retry a busy acquisition in
a loop needed for the drain to finish.

Reacquire depth 1, check exact target/generation and custody, commit outcome and
publish _info/external ref only on Ready. B_KEEP_LOADED is installed once on first
successful initialization. CleanFailure has proven no live resources and completed
ledger unwind; Closed has proven callback cessation, termination and all required
releases. Detach/release image only after those proofs and zero leases/pins/refs.
Quarantined retains metadata/image, all still-owned dependencies and uncertain
callbacks/DPC/context/mappings; already Released slots stay NULL and own nothing.
It preserves closed admission; publish no pointer and reject future get even with positive
internal count. If generation/custody mismatch occurs, retain everything and report
quarantine: do not guess a cleanup or repeat std_ops. No automatic replay/recovery.

## Required interleavings

| Scenario | Reservation and unlocked work | Commit and competing behavior |
| --- | --- | --- |
| First core ACPI get | Reserve core INIT/image; build dependency lease unlocked; run BusManager INIT unlocked, including balanced SMBIOS get/put. | Ready publishes only after exact generation commit; clean pre-subsystem error unwinds ledger; uncertain subsystem/cleanup failure quarantines. |
| Root or dependency-triggered get | Parent reserves and unlocks before dependencies/callback. Root obtains image lease, calls core get at unowned depth; core shares complete image lease and runs INIT. Hyper-V/display dependency path similarly enters child executor. | Child Ready creates a real reference owned by parent ledger/wrapper; parent failure releases it unlocked. Active-ancestor cycle returns busy, not a blanket rejection of dependency recursion. |
| Concurrent get while INIT/drain | Other thread briefly takes manager lock, sees transition before any positive-count shortcut. | B_BUSY, no output or new reference; never waits under manager/broker lock. Other targets may proceed. |
| Concurrent put during INIT | No new target external ref has been published. Sibling's existing refs remain independent. | Invalid target put cannot consume transition pin. Sibling put drops its own lease only; it cannot unload shared image or clear dependency pointers. |
| Concurrent puts at last ref | Serialize external count decrement/reservation. One thread swaps last ref into UNINIT pin. | Second invalid put returns error without consuming pin; get returns busy. No duplicate callback. |
| Drain worker get/put | Ready unrelated dependency get is allowed with brief lock ownership; target get is busy. Valid already-held target ref put is accepted while preserving pin (if drain was explicitly reserved with outstanding refs). | Snapshot decrements admission before final put. Commit cannot free image until all outstanding refs are gone. An admitted same-thread lifecycle request is rejected before drain. |
| Dependency release needs UNINIT | Prove image-wide callback cessation; clear DPC slot before its release with child ticket pin held. DPC ConsumedComplete discards its pointer and permits PCI release. | If PCI is uncertain, gDPC remains NULL/Released; PCI is NULL with retained ticket/child custody; device-manager stays Held/backed. Parent image remains quarantined and callbacks cannot run. |
| Dependency acquisition fails partway | Private device-manager/PCI records Held, DPC acquire fails; public slots never published. Reverse PCI release completes; device-manager release is uncertain. | PCI pointer discarded, DPC Empty, device-manager retained by exact ticket/reference or child custody. No callback or clean-failure publication. |
| INIT fails after callbacks started | Keep complete published set during stage cleanup. If callback cessation fails, release no dependency. | Proven cessation permits the same reverse slot protocol; partial release has the same NULL/retained rules, including consumed child uncertainty. |
| Missing safe top-level entry | Entry owns recursive lock, context allocation fails, unsupported execution context, or private lifecycle dispatch has no reservation. | Negative status before callback/ref publication/consumption; no skipped INIT reported as success and no arbitrary unlock. Caller must be converted, not automatically retried. |

Existing active refs ordinarily prevent last-put UNINIT; explicit future broker
drain with outstanding refs needs its own retained reservation, never falsified
ref counts. DPC/IRQ/notify cessation is a distinct proof from admission count zero.
No table mapping cleanup is assumed module-lock-free without transitive audit.

## Exact future production edits and validation boundary

- module.cpp: module/module_image definitions (:330–365), image unload/free
  (:380–484), dependency get/put (:665–708): add pin/lease/generation custody and
  tracked partial ledger, private acquisition slots, release tickets and atomic
  child custody transfer; protect every image teardown against those pins.
- module.cpp:714–829 init/uninit: replace locked callback execution with reserved
  executor; remove failure releases that bypass the ledger. :2180–2291 get/put:
  entry-depth gate, context scope, state-before-count checks, publication and
  separate reference/internal lifecycle outcomes. Audit remaining sModulesLock
  sites in preserved lock-entries.txt for nested calls and teardown bypasses.
- ACPI Module.cpp:267–284 wrapper: preserve balanced core get/put and route through
  new common executor; :31–40 dependency globals use shared image leases.
- ACPI BusManager.cpp:177–300: private stage-aware dispatch must return internal
  LifecycleResult without changing std_ops ABI. Manager selects the registered
  private ACPI adapter, which calls a typed ACPI lifecycle function; ordinary
  ACPI std_ops lifecycle calls without manager transition are rejected. Replace
  unconditional DPC deletion and log-only termination errors with retained outcomes.
  Registration/bootstrap and broker implementation remain future production work;
  no registration means lifecycle unavailable/error, never implicit successful INIT.
- Preserve Unsupported trusted-DMAR admission, IntelMemoryOwner.cpp:954–978 display
  setup, resident v0 intel_extreme.cpp:630/661/670 and callbacks outside Intel sLock.

Future production-helper tests must inject each row above, partial dependency
failure, same-image siblings, quarantine reacquisition, and DPC worker get/put.
Required future tests must specifically schedule DPC successful release followed
by PCI NotConsumed, ConsumedRetained, and unresolved-ticket outcomes; assert NULL
released/cleared slots, still-backed Held suffix, no replay, and zero callback
access after quiescence. Also inject acquisition-prefix failure and post-INIT
cleanup failure, active sibling lease, child callback parent reentry and slot-ticket
reservation failure. These are future production-executor tests, not fixtures run
here. This task runs no fixtures, compilation or runtime operations. TERRA must reproduce
source identities/caller searches and challenge every interleaving before ASTRA
acceptance. Host analysis proves neither GPU execution nor completion of any
remote operation. No hardware/lock change, deployment or cleanup; uncertain DMA
backing remains owned. Zero automatic recoveries used; protected failures stop.


## Bounded production release accounting (generation 121)

Patch 0106 routes public put_module through private put_module_internal. Its
module_release_result distinguishes NotConsumed (missing, quarantined, busy,
zero-reference or keep-loaded rejection), ConsumedComplete (one caller reference
released, including intermediate releases), and ConsumedRetained (reference
consumed but lifecycle teardown refused). The public status_t ABI and status
values remain unchanged. All decisions, decrement and retained-custody publication
occur under the existing recursive sModulesLock; this does not implement unlocked
callbacks, waiting or dependency cleanup.

ConsumedRetained carries the exact existing module pointer. The module and its
image receive lifecycle_retained before returning; existing guarded detach/reuse
paths preserve that identity without allocating after consumption. This private
identity is observational custody, not a new public reference or authorization to
release backing. Repeated release returns NotConsumed without replaying std_ops.
The production root Module.cpp acpi_module_std_ops and BusManager.cpp acpi_std_ops
remain unchanged and their nested lifecycle tests still exercise public put_module.
Host sanitizer results cannot establish GPU execution or callback cessation.
