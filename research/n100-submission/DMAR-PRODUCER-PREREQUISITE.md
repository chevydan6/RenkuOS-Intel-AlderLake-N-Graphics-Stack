# DMAR coherent-input producer prerequisite

Objective `5b82282cdef3c8bf6c56`, SOL generation 168. Design-only candidate on
published `e3a7bf98a8d47187d1401a336bb39b9ee74071f5`. No production change or admission.
Exact source identities: `tests/evidence/dmar-producer-design-168/sources.json`.
Pinned upstream revision is `4799bb5a188c4bb6bbef967287640cc21a061a0d`;
production overlays are the published parent patches, not the old worktree HEAD.
This narrows the earlier ACPI-SNAPSHOT-SYNCHRONIZATION.md mutation map; its
historical kernel-broker proposal is not an implemented facility or a request
for a replacement kernel. Current lifecycle retention patches supersede that
historical proposal for the present descriptor-only path.

## Finding: exact unavailable external guarantee

No inspected input boundary can currently meet the coherent acquisition contract.
AcpiTbInstallStandardTable in tbinstal.c:141 onward calls AcquireTempTable before
acquiring TABLES. tbdata.c:280–330 maps a header and initializes a descriptor;
InitTableDescriptor:130–161 records Table->Length and signature, and retains a
virtual pointer. AcquireTable:180–219 maps that recorded extent or returns that
pointer. Neither length has independent allocation provenance. Moving a copy
under TABLES would exclude descriptor mutation, not firmware, DMA or external
virtual writers during either initial header acquisition or copy.
ACPICAHaiku.cpp:494–550 maps requested physical ranges read/write; it supplies
accessibility, not allocation bounds or exclusion of other writers. Logical
Override:294–310 returns no kernel override; physical Override:329–334 returns
AE_SUPPORT. Neither creates an alternative trusted DMAR producer. Internal
virtual ownership does not prove coherent input, even if destination writes stop.

The missing external prerequisite is an authoritative source custodian that can
supply BOTH (a) a separately bounded accessible allocation containing the complete
DMAR, with length not derived solely from its header, and (b) exclusive stability
from before the first header read through the final byte copied, covering firmware,
DMA and every writable alias. Its evidence must bind address, extent, boot/source
identity, validity epoch, writer exclusion and expiry. A physical memory map or
ACPI reclaim/reserved designation alone does not establish (b). No such custodian
API or guarantee is established by the inspected pinned paths. This finding does
not claim exhaustive absence from uninspected firmware or bootloader sources.
Checksums, origin flags, module retention, TABLES and two equal copies cannot
supply the missing proof. A sealed destination only proves subsequent immutability.

## Conditional implementable boundary, disabled until prerequisite exists

Proposed private hook AcquireCoherentDmar(sourceToken, output) would live beside
AcpiTbInstallStandardTable, with an explicit custodian capability supplied before
AcpiTbAcquireTempTable reads anything. Existing callers without that capability
remain unsupported for trusted snapshots, while ordinary ACPICA installation and
display continue unchanged. Do not infer a capability from Address/Flags/Table.
A custodian independently validates source allocation bounds and acquires a
stability lease BEFORE any header access. Required size is 48..4096 bytes with
overflow-safe address+extent checks; header length must exactly equal that extent.
The producer alone writes one private 4096-byte destination while Constructing;
no ACPICA descriptor Pointer or writable alias to this destination is exported.
After copy and validation it transitions once to Sealed; permitted writers then
are none. Failed acquisition discards the unpublished destination. Source lease
is released only after the copy finishes; destination sealing cannot repair a
source lease absent during acquisition.

Associate a sealed slot with a nonwrapping descriptor-generation token under
TABLES after successful install/override resolution, not with a reusable array
index/address. Hooks at InitTableDescriptor, InstallTableWithOverride and
UninstallTable must create/invalidate associations, including replacement and
termination. Failed install cannot publish a slot. A duplicate may be suppressed
by ACPICA installation logic, so acceptance must verify the supported selection
semantics rather than promise detection of historical suppressed input tables.
Selection counts all current installed Address/nonzero DMAR signatures under
TABLES, including entries lacking provenance: zero Absent, second Duplicate,
unique without matching sealed token Unsupported. Never silently select the
only trusted entry among multiple descriptors.

Bound capacity: one current sealed 4096-byte slot, one in-flight acquisition,
and one caller-owned 4096-byte output. No heap growth or eviction of live data;
additional producers return Busy/Unsupported. Replacement invalidates admission
and defers slot reuse until readers drain; if custody is uncertain retain the slot
and fail closed. Epoch exhaustion, oversized inputs and exhausted capacity never
fall back to untrusted copying. Sealed bytes live until invalidation AND reader
drain; a returned owner copy has independent lifetime through its completion.

## Provider and owner transition

Preserve descriptor/v1, preflight/v1 and legacy owner ABIs. A future separate
snapshot/v1 module would export bounded CopyDmar(output[4096], capacity, report)
with actual length, descriptor epoch and status; return no internal pointer.
It uses the existing ACPI module dependency/retention model, whose final composed
shutdown semantics TERRA must confirm. Current retention leaves resources resident
on closure; it is not itself a stable-byte proof. Snapshot admission must test
ACPIReady and reject closure before ACPICA access. Any future real teardown must
close admission and drain admitted readers before TABLES destruction; no teardown
is enabled by this proposal. INIT/error/UNINIT remain governed by retention.

Order: owner sLock reserves request/operation pin then unlocks; acquire/record
provider dependency before calling it. Provider lifecycle spinlock is a short
state check, released before sleeping TABLES acquisition. TABLES protects selection,
generation and sealed-copy pin; no owner lock or custodian callback under it.
Custodian lease acquisition/copy occurs before TABLES, with lifecycle custody and
in-flight slot retained; recheck lifecycle and descriptor identity before seal
publication. Producer never holds TABLES while waiting on a source custodian.
Replacement/uninstall serializes on TABLES, invalidates the token, and reader
cleanup never decrements a newer generation. Shutdown closes first, then drains
outside locks or retains/quarantines on uncertainty. No callback under sLock,
lifecycle spinlock or TABLES. Keep acquired provider dependency resident on stale
completion, following actual PreflightDMAR custody; no speculative put/unload.

Future owner completion must recheck its lease, operation pin, DiagnosticAllowed,
provider epoch and snapshot validity under serialized completion. Closure or stale
completion cannot publish Configured or call ConfigureReserved; retain uncertain
request/provider/backing. A validity fence spanning selection through completion
is required: provider grants a read lease, replacement cannot commit while it is
held; release happens after owner publication outside sLock. Replacement that
would invalidate configured policy must close further owner admission and retain
existing backing pending separately reviewed retirement. This notification and
lease protocol is an additional implementation prerequisite, absent from v1.

## Actual parser path and limits

On an independently coherent owned output, future validation calls
intel_dmar_inventory (IntelDmarInventory.h): signature, exact 48..4096 length,
checksum, record/scope bounds and at most 16 units. It does not enforce all header
revision/reserved-field policy: the future producer must specify and check those
separately, never claim current parser does. intel_dmar_reserved
(IntelDmarReserved.h) additionally rejects unknown scopes, unsupported unit flags,
non-DRHD/RMRR structures, over 16 RMRRs, bad RMRR reserved fields, alignment and
scope paths. It returns a local result only on success. ConfigureReserved
(IntelN100KernelPages.h:108) repeats this real parser, rejects active/live/ambiguous
state, then copies ranges and sets configured. Its input must stay owned and
stable for that call. Current PreflightDMAR never calls it and always reports
configured=false; a unique descriptor stays Unsupported. This proposal does not
add any caller, authorize BeginCpuBacking, create areas, map devices, publish
caches, submit commands or prove DMA retirement/GPU execution.

## Independent acceptance obligations (future production entry points)

These are required tests, not executed fixtures. TERRA first verifies source claims
and feasibility; without the custodian guarantee only rejection is implementable.

| Entry / injection | Required observation |
| --- | --- |
| AcquireCoherentDmar with mutable physical/external/internal source, no capability | Unsupported before header/copy; zero slot publication, ConfigureReserved and area calls. Matching checksums or double-copy agreement changes nothing. |
| Acquisition with trusted extent outside 48..4096, overflow, short allocation or unequal header length | Reject before out-of-bounds read; no sealed token; lease released exactly once after any admitted copy. |
| Install then classify/copy with zero, one unproven, two installed DMAR descriptors | Absent, Unsupported, Duplicate respectively; second unmappable descriptor still counts; no untrusted read. |
| Override, uninstall/reinstall, resize while producer pauses before publication | Old generation never binds new descriptor; pending slot discarded or retained on uncertainty; no index identity reuse. |
| Copy read lease held across replacement | Replacement cannot commit until completion lease drains; stale epoch rejects; no old policy applied to new generation. |
| UNINIT before admission / during acquisition / after copy | Reject before ACPICA, or retain admitted custody until cleanup; no TABLES destruction with active reader; no successful reopening. |
| Owner close while snapshot callback pauses | Real PreflightDMAR successor returns incomplete/busy, configured false; no ConfigureReserved/BeginCpuBacking/area calls; request, provider and uncertain backing retained. |
| Capacity, epoch exhaustion, mutex/dependency failure, parser rejection | Distinct fail-closed status; no fallback, no partial policy, no leaked/replayed lease or counter decrement in another epoch. |
| Valid coherent fixture into real parser and ConfigureReserved | Exact expected RMRR ranges; live/active/ambiguous rejects; zero BeginCpuBacking, allocation and DMA operations. |

No builds, runtime tests, hardware/VM access, lock changes or deployment were run.
This task resolves the design question by naming the unavailable guarantee and
conditional integration requirements; it does not assert the prerequisite exists.
