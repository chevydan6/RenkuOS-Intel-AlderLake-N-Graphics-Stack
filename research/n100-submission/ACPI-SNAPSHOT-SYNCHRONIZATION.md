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
