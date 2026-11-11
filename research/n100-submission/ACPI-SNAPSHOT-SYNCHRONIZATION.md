# Pinned ACPI snapshot synchronization and stability audit

Objective `0e6247b7d991d96fbe80`, SOL generation 72 (corrects generation 70 reference accounting). Design-only refinement of
TRUSTED-DMAR-CONTRACT.md, based on published parent
`afddb1fbcad112b4929de837afb5cf866451254e`. No provider is implemented or approved.

## Finding and reproducible scope

The table mutex can protect a descriptor during a bounded operation in a live,
initialized subsystem. It does **not** prove immutable backing bytes or subsystem
lifetime. Therefore the proposed provider must currently return Unsupported for
DMAR admission: no pinned origin establishes the required byte-stability guarantee.
This is a resolved fail-closed design outcome, not a successful snapshot proof.

Evidence: `tests/evidence/acpi-synchronization-070/sources.json` records pin
`4799bb5a188c4bb6bbef967287640cc21a061a0d`, source repository, exact Git blobs,
SHA-256 values and the executable full-subtree search command. `coverage.txt`
retains every match, including callers outside tables; `pinned/` retains every
matched file and the complete tables directory. Re-run that command against the
local object store and compare bytes. No network or alternate baseline was used.
Paths below are relative to src/add-ons/kernel/bus_managers/acpi; line numbers
refer to those pinned bytes. Coverage is source coverage, not a claim to exclude
firmware, DMA, arbitrary physical aliases or external virtual owners.

## Mutation and ownership map

| Operation | Pinned definition and callers; synchronization |
| --- | --- |
| Initial array/root installation | acpica/components/tables/tbxface.c:67–76,105 onward: AcpiAllocateRootTable/AcpiInitializeTables initializes root state before normal use; tbutils.c:274 onward parses root and calls InstallStandardTable. Initialization is a lifecycle prerequisite, not covered by a reader's table lock alone. |
| Descriptor install/override | tbinstal.c:70–111 allocates a slot and initializes it; :141–231 InstallStandardTable obtains temporary header before TABLES, verifies/installs under TABLES, drops lock for notification, reacquires then releases temp. tbdata.c:130–161 zeroes descriptor and copies header Length/signature. Callers include tbxfload.c:312,346 and namespace table loading; complete callers in coverage.txt. |
| Removal/replacement | tbinstal.c:336–363 invalidates, frees INTERNAL_VIRTUAL, clears Address without consulting ValidationCount. Callers: override :307, temporary duplicate :199, tbutils.c:171 DSDT copy, tbxface.c:240 reallocation verification, tbdata.c:826 termination. Reference count alone cannot prevent uninstall. DSDT replacement is invoked from tbxfload.c:193 while TABLES is held; it is not a DMAR copy guarantee. |
| Resize | tbdata.c:677–744 allocates, compacts descriptors, frees old array, replaces root fields; :762–795 allocates next slot and may resize. tbxface.c:76 initialization caller lacks normal runtime locking; :247 reallocation holds TABLES (:198–250). No descriptor pointer/index may survive unlock across a resize. |
| Mapping | tbdata.c:180–219 AcquireTable maps physical recorded extent, uses existing pointer for either virtual origin; :238–250 releases only physical mappings. :280–330 temporary acquisition maps header, derives descriptor extent from header, unmaps header; :375–431 Validate/Invalidate stores/releases mapping. Recorded Length is not an independently authenticated allocation extent. |
| References | tbutils.c:425–466 GetTable validates on count zero and increments with saturation; :488–530 PutTable decrements and invalidates on zero. tbxface.c:391–413 and :452–469 public Get/Put acquire TABLES themselves. Private get/put require caller-held TABLES for root entries. Reject ValidationCount >= MAX-1 under TABLES before Get, where MAX is ACPI_MAX_TABLE_VALIDATIONS; rejecting only MAX is insufficient. |
| Unload | tbxfload.c:433–544 routes unload to AcpiTbUnloadTable; table namespace load/unload paths and root metadata writes are retained in coverage. Namespace unloaded state is not proof of descriptor removal or backing immutability. |
| Termination | tbdata.c:811–844 takes TABLES, uninstalls each entry and frees root array; utilities/utinit.c:341 calls it during shutdown. utilities/utmutex.c deletes mutex objects during termination. A reader cannot safely race acquisition against mutex destruction merely by holding an ACPI module reference. |

BusManager.cpp:851–855 is the lossy public get_table wrapper. Existing module
retention does not establish a shutdown exclusion protocol. The audit search also
captures interpreter/namespace/executer callers and direct root list accesses;
those cannot be replaced with assumptions that all accesses use public GetTable.

## Separate backing proof

Physical origin: ACPICAHaiku.cpp:494–550 maps with kernel read/write permissions;
:568–572 deletes the mapping's area. It establishes accessibility, not ownership,
immutability, exclusion of firmware writes or physical aliases. Descriptor Length
originates in an earlier header read. Missing guarantee: independently trusted
accessible extent and stable bytes across extent/header acquisition and copy.
Reject physical DMAR until such a guarantee is furnished by a reviewed producer.

External virtual origin: caller owns bytes; tbxfload.c:312/396 accepts caller
storage and tbdata.c:130–161 retains its pointer. TABLES cannot exclude caller
writes/frees. Reject without a producer lifetime and immutable-byte contract.
Internal virtual origin: ACPICA owns allocation/free, but the origin flag alone
does not certify allocation extent or absence of aliases/writers; DSDT copy is
specific to DSDT (tbutils.c:145–180). No pinned DMAR immutable-copy producer was
established. Reject this origin too without explicit provenance and stability.

Overrides: tbinstal.c:249–319 supports logical/external-virtual and physical
replacement, verifies then uninstalls/reinitializes. ACPICAHaiku.cpp:294–310 returns
no logical override in kernel configuration (ACPI_EXEC_APP differs); :329–334
returns AE_SUPPORT for physical override. Do not generalize those stubs into an
immutability guarantee, and reject any future override without the same proof.
Checksum, double-copy comparison and repeated header reads cannot prove atomic
snapshot consistency against an uncoordinated writer; none substitutes for it.

## Conditional provider sequence and resource accounting

This sequence specifies a future seam, conditional on separately reviewed
lifecycle and immutable-backing contracts. Today fail Unsupported before mapping.

1. Caller takes owner sLock only to reserve its lease/pin and Configuring gate,
   then unlocks. Acquire/record provider module dependency outside sLock.
2. Provider admission must exclude shutdown and initialization for the entire
   operation using a lifecycle gate honored by shutdown before mutex destruction.
   Such a gate is a missing facility, not an existing proven lock. Required order:
   lifecycle admission, then TABLES; never sLock. Shutdown closes admission, drains
   calls, then terminates. No callbacks while holding either owner lock.
3. Check TABLES acquisition status; enumerate at most captured CurrentTableCount
   under that mutex, using installed Address plus descriptor Signature. A second
   DMAR is Duplicate even if unmappable; do not map to count signatures. Empty
   list is Absent. No pointer/index escapes this critical section.
4. For unique selection require reviewed stable origin/provenance, independently
   accessible descriptor extent 48..4096 and ValidationCount <
   ACPI_MAX_TABLE_VALIDATIONS - 1. Check this guard under TABLES before any Get
   or mapping; reject MAX-1 and MAX as Unsupported without changing the descriptor. Use
   private AcpiTbGetTable directly under TABLES, never recursive public Get/Put.
   On mapping failure return dependency/NoMemory with no acquired reference.
   On success account exactly one temporary reference; retain it through copy.
5. Within trusted extent read header, require declared length equals recorded
   extent, signature DMAR, bounded exact copy into preallocated 4096-byte staging.
   Immutable producer guarantee must cover these reads. Check copied checksum,
   supported revision/reserved fields and actual IntelDmarReserved parser.
6. Every successful get has exactly one private Put under the same TABLES hold,
   including validation/copy failure. Failed get has zero puts. Counts >= MAX-1 are
   rejected before get; the maximum admitted initial count is MAX-2, so the
   temporary count never reaches the permanent saturation value. Unlock TABLES, end lifecycle admission; return only copy,
   status and provenance, no descriptor/table pointer. Caller performs parser
   work outside TABLES if moved after release; immutable staging remains owned.
7. ConfigureReserved (IntelN100KernelPages.h:108) precedes BeginCpuBacking. Recheck
   owner generation/close/gate under sLock before publication; stale result creates
   no areas, preserves retained record/dependencies and cannot publish Configured.

## Reference-count boundary proof and required future seam assertions

Here MAX = ACPI_MAX_TABLE_VALIDATIONS = ACPI_UINT16_MAX (65535), pinned
actbl.h:425. Get (tbutils.c:439–455) increments a count below MAX; Put
(:488–513) decrements only below MAX. The old unsaturated-only guard admitted
65534, producing 65535 permanently: one Get/Put call pair is not sufficient.
With the new guard, initial c is in [0, MAX-2]; successful Get produces c+1
in [1, MAX-1], so exactly one Put restores c. Keep TABLES continuously held
from guard through Put, with no intervening callback or recursive reference
operation. Never repair counts by direct decrement or Put a rejected/failed Get.

The following are required assertions against the future production seam,
not executed fixtures or evidence that a provider currently exists. Every case
also asserts no escaped pointer, no published bytes on failure and no area calls.
All cases remain conditional on the missing lifecycle/immutable-byte proofs.

| Initial state / injection | Get / Put calls and count transition | Mapping ownership and result |
| --- | --- | --- |
| MAX-2 (65533), valid descriptor | 1 / 1; 65533 -> 65534 -> 65533 | Existing mapping unchanged; no map/unmap. Copy success may return owned staging only. |
| MAX-1 (65534) | 0 / 0; unchanged 65534 | Reject Unsupported before mapping/copy; existing mapping untouched. |
| MAX (65535) | 0 / 0; unchanged 65535 | Reject Unsupported; do not attempt to unsaturate or release preexisting ownership. |
| 0, NULL pointer, physical map fails | 1 failed / 0; remains 0 | One failed map attempt, zero acquired mappings and zero unmaps; return dependency/NoMemory, not Absent. |
| 0, NULL pointer, physical map succeeds | 1 / 1; 0 -> 1 -> 0 | One acquired mapping; final Put invalidates, unmaps once and clears physical Pointer; owned staging only escapes on success. |
| 1, existing valid physical mapping | 1 / 1; 1 -> 2 -> 1 | No map/unmap; preexisting reference and Pointer retained. |
| 0, existing valid virtual pointer | 1 / 1; 0 -> 1 -> 0 | No physical map/unmap; Put does not free producer storage. Separate producer lifetime remains mandatory. |
| Any admitted count, header/copy/parser failure after successful Get | 1 / 1; c -> c+1 -> c | Discard staging; if c=0 physical mapping, unmap once, otherwise retain preexisting mapping/storage. |

Mapping-failure accounting follows tbdata.c:180–219: Acquire returns
AE_NO_MEMORY before writing outputs when mapping returns NULL. Validate
(:375–399) propagates failure with Pointer still NULL; Get returns before
increment and before assigning OutTable. Initialize the provider's local output
to NULL and never consume it on failure. Put-at-zero would underflow to MAX,
so failure must not flow into the successful-reference cleanup path. For c>0,
Get skips Validate; the conditional provider must require a valid existing
Pointer/provenance and reject an inconsistent descriptor without Get/Put.
Invalidate/Release (:238–250, :415–435) unmaps only physical origin; restoring
count does not imply freeing virtual storage. Mapping success followed by any
copy/validation error uses the same single-Put cleanup as successful copy.

## Later production-seam interleavings (not executed tests)

| Pause/injection | Required result and accounting |
| --- | --- |
| Before selection, install second DMAR | Installer serializes through TABLES; snapshot sees either serial order; second descriptor means Duplicate with zero gets/maps. |
| During selected copy, resize/uninstall | Mutator waits on TABLES; one get/put, no escaped descriptor; after unlock mutation cannot affect staging. Startup/reinit must be excluded separately. |
| Mapping fails | Initial count 0/NULL Pointer: one failed Get, unchanged count 0, no put/unmap, zero published bytes/area calls; status distinct from Absent. |
| Header mismatch/parser error | One get/put if acquired; staging discarded; zero ConfigureReserved success/areas. |
| Uncoordinated firmware/external writer | Unsupported before touching bytes absent immutable-producer proof; test must not treat matching checksums as stability. |
| Shutdown before admission/during copy | Gate rejects before mutex use, or shutdown waits until put/unlock/drain. Without gate proof provider remains Unsupported. |
| Close/reentry during provider callback | Callback observes no sLock; close invalidates publication, no backing release or new area; successful dependencies remain resident on uncertainty. |

Applied production references remain those of PINNED-RESIDENCY-BASELINE.md:
IntelMemoryOwner.cpp:954–978 StandardOperations retains PCI/GART and initializes
sLock; no new ACPI dependency in display INIT. intel_extreme.cpp:630,661,670 v0
release/free/allocate dispatch is unchanged. Custody provider is lazy, outside
sLock, gated before ConfigureReserved and area creation. All runtime code is
unchanged. TERRA must independently check coverage and this conditional proof;
ASTRA alone chooses/accepts the next implementation direction. No builds, tests,
hardware, deployment, replay or lock changes occurred. ASan was not run; future
runs require ASAN_OPTIONS=detect_leaks=0 and disclosure of disabled leak checks.
Uncertain GPU DMA backing remains owned; installed equivalence/recovery gates
remain mandatory for any separately reviewed physical procedure.

## Concrete lifecycle integration design (objective 61905358628ee4e2868e)

SOL generation 77; corrects generation 75 owner-thread reentry; design only, based on published 931023da7c3e687a45bd89a53d5b403f1f97ec76.
This section replaces the unspecified lifecycle facility in step 2 above. It does
not establish immutable backing, implement a provider, or authorize admission.

### Pinned caller coverage and cleanup asymmetry

`tests/evidence/acpi-lifecycle-075/sources.json` pins and hashes each matched file
from a reproducible Git grep across the entire pinned `src` and `headers` trees.
`coverage.txt` retains all 71 matches (12 files), including declarations/comments.
This finds one production initialization caller sequence, BusManager.cpp:219,
223,242,250,257, and two AcpiTerminate call sites, :283 error cleanup and :294
UNINIT. No other direct lifecycle caller appears in that scope. This does not
prove absence of indirect calls, external modules or out-of-tree callers.
Module.cpp's B_KEEP_DRIVER_LOADED flag is not used as a lifetime proof.

utxfinit.c:76–138 initializes OS, globals, mutexes, namespace and interfaces,
returning immediately on each failure. BusManager.cpp:221 goes directly to
err_dpc on InitializeSubsystem failure; subsequent failures go to err_acpi.
Thus a subsystem-init failure may leave partial resources without AcpiTerminate.
Existing code is not a proven reusable partial-initialization unwinder.
utxface.c:67–86 always calls SubsystemShutdown, MutexTerminate and OsTerminate.
utinit.c:311–315 only guards SubsystemShutdown with AcpiGbl_Shutdown; it does
not suppress the later mutex/OS teardown in AcpiTerminate. utmutex.c:75 onward
can fail partway through mutex/lock creation. A repeated Terminate therefore
cannot be justified merely by that global flag. Neither flag is admission.

### Storage, state and exact prospective hooks

Choose a **kernel-resident broker**, not an ACPI-module static gate. Add future
`src/system/kernel/acpi_snapshot_gate.cpp` to KernelMergeObject in kernel/Jamfile
(adjacent to module.cpp); expose a private kernel API, with export/build wiring
reviewed before implementation. The pinned lock.h static MUTEX_INITIALIZER
supports a kernel-lifetime control mutex. Broker state is boot-static and never
destroyed/reset by AcpiUtInitGlobals, AcpiTerminate, module UNINIT or reload.
This is an explicit new production dependency; it is absent today. No replacement
kernel is proposed or built by this task.

Broker owns control mutex C, state {Closed, Initializing, Open, Closing,
Quarantined}, nonwrapping epoch, active count and a single lifecycle transaction
owner. Only short state/count updates occur under C. No ACPICA pointer or module
function pointer is kept in broker storage. Snapshot executes via the caller's
normal valid module entry; stale arbitrary function pointers after module unload
are outside this protocol and remain a separate module-ABI requirement. The
protocol does not infer shutdown exclusion from that reference.

* BusManager.cpp:181 INIT: before CopyDsdtLocally/settings/DPC/any ACPICA access,
  claim lifecycle owner under C, requiring Closed and active=0. Set Initializing,
  allocate a fresh epoch; reject concurrent INIT/UNINIT as Busy without ACPICA
  calls. Never wrap epoch; exhaustion quarantines. Disabled-ACPI or DPC failure
  before subsystem invocation returns Closed and releases owner.
* Before :219 record subsystem invocation. Any failure from this call takes a
  new quarantined failure exit, still deleting the owned DPC queue as appropriate,
  but never reopening admission or retrying initialization. Partial ACPICA cleanup
  is not guessed: preserve uncertain resources until a separately reviewed
  stage-aware unwind exists. This changes proposed retry policy, not current code.
* At :280, immediately before successful B_OK after final initialization work,
  publish Open under C and release lifecycle owner. Failure after successful
  InitializeSubsystem (:223–259) retains Initializing (no snapshots admitted),
  routes through the same termination wrapper as normal teardown, then deletes
  DPC. Any uncertain cleanup result leaves Quarantined, never Closed.
* Adjacent to get_table at :851, add a separate bounded snapshot API (do not
  silently change the existing lossy wrapper). Under C test Open and epoch,
  reject counter overflow, increment active and create a call-local token. This
  increment is the admission linearization point. Release C before TABLES.
  No rejected path calls any ACPICA function, including status/mutex helpers.
* BusManager.cpp:282 err_acpi and :292 UNINIT use broker lifecycle transactions.
  Normal UNINIT atomically sets Closing under C **before** AcpiTerminate and DPC
  deletion. Wait for active=0 without holding C or any ACPICA/owner lock. Use
  bounded sleep/poll with C rechecks, not spinning or a destructible ACPI event.
  Timeout leaves admission closed, resources retained and Quarantined; no teardown,
  DPC deletion or unload-success report is permitted. A competing transaction
  returns Busy rather than performing duplicate cleanup.
* Route both callers through one Haiku termination wrapper; additionally require
  a Haiku-specific entry check at utxface.c:73 before :78 (SubsystemShutdown).
  At utxfinit.c:82 require the corresponding INIT check before any OS/globals
  initialization. The owner token and active=0 alone are **not** authorization.
  Each check must atomically consume the operation-specific, single-use entry
  permit below, before executing any ACPICA entry body. A rejected call returns
  failure without reading or modifying ACPICA state. Never recursively drain.
  Private shutdown/mutex helpers remain internal to that authorized body; direct
  public access to private teardown is forbidden and must be audited before enablement.

### Single-use entry authorization for no-argument lifecycle APIs

Broker storage additionally owns a permit record {epoch, transaction serial,
owner thread identity, operation, phase}. Operations are INIT_SUBSYSTEM and
TERMINATE, never interchangeable. Phases are Unarmed, Armed, Executing, Returned
and Failed. Serial allocation never wraps; exhaustion quarantines. Owner identity
must identify the live thread incarnation, not a recycled numeric ID; the owning
synchronous wrapper retains its lifetime until transaction release. Kernel API
and thread-identity plumbing remain prerequisites, not existing guarantees.

Only the broker's internal wrapper may arm a permit. Its acquire/arm API is not
an exported general capability usable by callbacks. After arming, the wrapper
immediately invokes the designated no-argument ACPICA API, with no intervening
callback or dispatch. These are synchronous calls on the owner thread; asynchronous
entry is rejected. The API's first Haiku-specific instruction calls the resident
broker's TryEnter(operation), which looks up the sole transaction record under C;
no thread-local ambient token supplied by the caller is trusted. Require matching
live owner thread, epoch/serial, correct operation, phase Armed, active=0 and the
appropriate state (Initializing for INIT; Closing or initialization-error cleanup
for TERMINATE). In that same C critical section change Armed to Executing. That
transition is the entry linearization point. Release C before any ACPICA body.
Wrong thread, wrong operation, missing permit, Executing/Returned/Failed or stale
transaction all reject without mutating the permit or touching ACPICA state.

The outer wrapper alone finalizes Executing after the synchronous API returns,
including each early error return: record Returned on success or Failed on error.
No early return restores Armed. A phase mismatch or abnormal/missing return means
quarantine with resources retained; never infer completion, replay or clear owner.
The API body and callbacks cannot rearm, finalize or mint permits. The same
operation is never armed twice in a transaction. INIT success does not authorize
TERMINATE: after INIT has returned successfully, a later initialization failure
may arm a distinct TERMINATE permit once, via the outer error-cleanup wrapper.
An INIT failure instead quarantines without speculative teardown. Normal UNINIT
uses a new transaction after closure/drain. Clean reinitialization uses a new
epoch/serial; no previous permit is reused. Remaining initialization calls run
under the lifecycle owner with no Armed permit; direct INIT/TERMINATE calls from
their callbacks fail too. Competing lifecycle wrappers cannot claim ownership.

Enforce rejection regardless of whether pinned callbacks actually reenter. The
shutdown's event/namespace calls (utinit.c:326–341) and subsystem initialization
calls (utxfinit.c:76–138) are treated as potentially reentrant; no absence-of-callback
claim is needed. This only designs the two guarded entry points. Coverage of
private helper callers, exports and other ACPICA APIs remains a pre-enable audit.

After successful complete termination, delete DPC while retaining transaction
ownership; under C set Closed and release owner. Repeated UNINIT in Closed is
an idempotent no-op, with no ACPICA or DPC call. A clean next INIT may claim a
new epoch. Closed is published only following proven complete teardown, never
on partial-init failure, timeout or uncertain OS termination. Quarantined has
no automatic reinitialization/cleanup escape. The stage-aware partial-init
unwinder and kernel exports are unresolved implementation prerequisites, explicitly
not supplied by a module reference or this design document.

### Ordering and interleavings

Snapshot holds no C while acquiring/holding TABLES. On every path after admission,
finish successful private Get with exactly one private Put, then unlock TABLES,
then under C decrement active using its epoch token exactly once. A TABLES
acquisition failure still releases admission, with no Get/Put or mutex unlock.
Use structured cleanup, never a timeout that discards an executing token. Release
of TABLES precedes publication of active=0; mutex synchronization through C gives
teardown a happens-before edge. Tokens cannot survive into another epoch.

Lock ordering: caller reserves under sLock then releases it; broker C is short
and unnested with TABLES/sLock; admitted operation takes TABLES only. Shutdown
holds no C/TABLES/sLock while draining or invoking ACPICA teardown. No callbacks
under sLock, C or snapshot TABLES; any required callback occurs outside those
locks, with module/custody ownership retained. Reentrant termination during an
admitted snapshot must fail Busy (detect call-local admission) rather than wait
for itself. Lifecycle INIT/teardown callbacks cannot acquire another lifecycle
transaction. Existing ACPICA internal callbacks require their own audit; this
protocol protects the prospective snapshot, not all unrelated ACPICA APIs.

| Interleaving | Required order and outcome |
| --- | --- |
| Shutdown wins C before admission | Closing is observed; reject without querying ACPICA globals, TABLES handle or status. Mutexes may already be gone. |
| Admission wins C, pauses before TABLES | active=1; shutdown closes but cannot call SubsystemShutdown/MutexTerminate until that token drains. TABLES still exists. |
| Shutdown during copy or error cleanup | Closing rejects new calls; admitted call performs Put if acquired, unlocks TABLES, decrements active. Only then teardown starts. |
| Snapshot pauses after unlock before decrement | Teardown still waits; no premature destruction. |
| Drain deadline expires | Quarantined, no teardown/unload success; retain resource ownership, do not force active to zero. |
| Owner-thread callback during INIT body calls INIT or TERMINATE | INIT permit is Executing, so nested INIT fails; TERMINATE has no Armed permit and fails. Neither reaches globals/OS initialization or teardown; outer INIT retains its permit and runs to return. |
| Owner-thread callback during TERMINATE body calls TERMINATE or INIT | TERMINATE is Executing, so nested teardown fails before SubsystemShutdown; INIT operation mismatch fails. active=0 and matching owner do not authorize either call. Outer teardown alone reaches mutex/OS destruction once. |
| Same-thread repeat after body return, before owner release | Permit is Returned/Failed, never Armed; repeat rejects. INIT-to-cleanup needs separately armed TERMINATE after successful INIT; reverse/nested transition is forbidden. |
| Competing thread while Armed or Executing | Owner-thread mismatch rejects without consuming permit; competing wrapper cannot acquire transaction. Correct owner alone consumes Armed. |
| Two terminators / repeated termination | Single-use TERMINATE permit allows one body; competitor Busy; Closed repeat is no-op. AcpiGbl_Shutdown is never the guard. |
| INIT fails before/inside/after subsystem init | Pre-invocation failure returns Closed; inside failure quarantines; later failure uses authorized cleanup with admission never Open. |
| Clean terminate then reinitialize | old tokens drained before Closed; new epoch opens only after complete new initialization. No stale token can reopen or decrement new state. |

Required future production-seam checks pause at each row's boundaries, including
TABLES acquisition failure, failed Get, copy rejection, token overflow, epoch
exhaustion, teardown failure and reentry. These are acceptance obligations, not
new reference fixtures or executed tests. Inspect actual kernel API/export and
unload-error semantics before implementing; if those cannot preserve retained
resources on quarantine, do not enable the seam. Pinned sources do not establish
that system-wide guarantee today.

IntelMemoryOwner.cpp:954–978 display INIT stays PCI/GART+sLock only; v0 driver
release/free/allocate dispatch remains unchanged. Provider callbacks stay outside
sLock. ConfigureReserved/BeginCpuBacking remain unreachable from untrusted DMAR:
Unsupported admission continues until independent accessible extent and immutable
backing contracts are supplied. Lifecycle exclusion alone advances no GPU execution,
DMA retirement or installed-artifact claim. No production edits, compilation,
hardware operations, replay, deployment or canonical lock changes occurred.
