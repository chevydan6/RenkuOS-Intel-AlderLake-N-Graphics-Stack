# ACPI module failure retention and drain ordering

Objective `aa7a345c71ef0b48161a`, SOL generation 80. Design only, based on
published `a5380e9d1ee90b7d096a9510fd9652a4b18ea498`. This supplements the
single-use lifecycle permit design; it does not implement or enable that seam.

## Pinned evidence and finding

`tests/evidence/acpi-module-retention-080/sources.json` pins source revision
`4799bb5a188c4bb6bbef967287640cc21a061a0d`, repository, Git blobs and SHA-256
for module.cpp, elf.cpp, module.h, ACPI declarations/OS bridge/table helpers and
DPC implementation. `pinned/` contains those exact bytes. `coverage.txt` retains
all matches from the recorded executable git-grep command. Coverage includes the
whole ACPI/DPC subtrees and both kernel files; it is not an assertion that no
other kernel or external caller exists. Reproduce via the JSON argv, then compare
stdout and pinned blobs. No network access is needed.

The current manager cannot retain arbitrary failed INIT resources safely through
an ordinary negative status. A resident admission broker cannot fix that contract.
Moreover a module reference is not sufficient shutdown exclusion. ACPI declares
B_KEEP_LOADED in BusManager.cpp, whereas Module.cpp's B_KEEP_DRIVER_LOADED is a
device-node attribute. Neither flag proves partial-INIT retention.

## Actual paths and separate ownership

Locations below are relative to the pinned tree; module.cpp line references
refer to src/system/kernel/module.cpp.

| Case | Actual manager/ACPI behavior and necessary outcome |
| --- | --- |
| Pre-subsystem failure | BusManager.cpp:201–212 returns ENOSYS/B_ERROR before ACPICA subsystem invocation. Manager :725–745 acquired automatic dependencies, then releases them on callback failure. get_module :2232 releases the image if module refs remain zero. Safe only after proving no live resource/callback was created; no quarantine needed for a proven empty failure. Dependency acquisition failure :698–705 itself does not unwind earlier acquired entries; any redesign must track precisely which dependencies were acquired. |
| Partial subsystem INIT failure | BusManager :219–221 jumps to DPC deletion :285–289 and returns B_ERROR; no AcpiTerminate occurs on this branch. Manager :744 releases device-manager/PCI/DPC references and :2232 may release executable image. Unknown ACPICA resources, handlers, mapped areas and queued callback code/context cannot be protected by broker state. Required quarantine must retain image and acquired dependencies BEFORE this release path and leave queue/resources owned, not delete them speculatively. |
| Later initialization failure | BusManager :223–259 routes to :282, logs AcpiTerminate failure, always deletes DPC and returns B_ERROR. Even failed cleanup therefore reaches INIT's destructive release path. A stage-aware wrapper must distinguish fully clean failure from uncertain cleanup; uncertain means quarantine with code, callbacks, DPC and dependencies retained. |
| Drain timeout / failed UNINIT | Proposed timeout must return negative without termination or queue deletion. Actual uninit_module :817–819 sets MODULE_ERROR, B_KEEP_LOADED and adds one retained reference; no put_dependent_modules runs, and put_module :2273 skips image release. However put_module :2290 returns B_OK regardless of uninit status. Current ACPI :292–300 always returns B_OK after logging errors, so this retention is not reached today. |
| Successful teardown | Only proven complete teardown, including callback cessation and successful DPC destruction, may report B_OK. uninit_module :808–811 releases dependencies; put_module then releases the image at zero refs. ACPI's B_KEEP_LOADED creates a permanent reference on successful first get (:2220), so ordinary balanced puts do not normally reach ACPI UNINIT at all. Direct std_ops calls bypass manager ownership and remain forbidden for the new seam. |
| Repeated acquisition | Critical hole: after failed UNINIT the retained ref is positive. get_module :2216–2224 bypasses init_module and returns B_OK with the module pointer, despite MODULE_ERROR. Thus :763's ERROR rejection does not protect that case. A quarantine design must reject ERROR/quarantined/in-progress state BEFORE positive-ref fast path; retain internal pin without exposing another callable acquisition. |

Module.cpp:Module image ownership is distinct from module count: :360–363,
:470–484, :2180–2235. At the last image reference after boot-device availability,
unload_module_image :448–466 calls unload_kernel_add_on and frees metadata;
elf.cpp:2409–2419 takes image locks and invokes unload_elf_image :1147–1155,
which unregisters/deletes at final ELF reference. Preboot suppression is temporary,
not a retention proof (module_init_post_boot_device:1850 onward removes unused
images). A different module in the same image may incidentally retain code, but
cannot justify retaining this module's dependency set or resource lifetime.

Module.cpp (ACPI):31–40 declares device-manager, PCI and DPC automatic dependencies
shared by the image's exported modules; :acpi_module_std_ops acquires/releases
B_ACPI_MODULE_NAME through get/put. SMBIOS in BusManager:229–238 is a separate
balanced temporary reference, not automatic quarantine custody. The broker must
store no module-owned callable pointer after release. Existing asynchronous
AcpiOsExecute (ACPICAHaiku.cpp:860–882) queues function/context via gDPC, so merely
retaining counter storage cannot preserve the target code/context. DPC deletion
(dpc.c:128–154) waits for its worker before freeing queue storage; calling it can
block on callback completion. No claim is made that it completes safely on an
arbitrary partial ACPICA initialization.

## Module lock and a viable ordering

get_module :2191 and put_module :2248 hold recursive sModulesLock across
init_module/std_ops and uninit_module/std_ops, including dependency get/put.
Recursion permits same-thread manager calls (e.g. SMBIOS), not other-thread
completion. Never assume an admitted snapshot or DPC can finish while this lock
is held: its outer normal module-reference put needs it. An implementation which
counts admission through that put deadlocks against UNINIT's drain. Even if the
private table Put -> TABLES unlock -> broker decrement path itself has no direct
get/put call (tbutils.c:488–530, tbdata.c:238–250), a mapping cleanup/allocator path
or callback needs a separate transitive lock audit. This source analysis does not
certify all such paths lock-free with respect to the module manager.

Required ordering for the future seam: reserve an image/dependency retention pin
and exclusive lifecycle transition under sModulesLock; close broker admission
under C; retain the transition/pin, then release ALL recursion depth of the module
lock before draining, invoking ACPICA termination or waiting for DPC. Do not just
unlock once from std_ops: callers may already own recursive depth. Manager-owned
split-phase dispatch is required; entry from recursively nested manager callbacks
must defer/reject before changing lifecycle state. No C, TABLES or Intel sLock is
held across waits/callbacks. Snapshot finishes private Get/Put, unlocks TABLES,
decrements admission under C, then drops its ordinary module reference; the
manager pin covers the interval. Admission completion must require no module
lock. Reacquire sModulesLock only to commit outcome after worker return; no
callbacks execute under it in this proposed split-phase operation. Competing gets
must fail busy/quarantined, never wait while owning another lock needed to drain.
Same-thread admitted teardown rejects Busy rather than self-draining. Existing
callers/dependency transactions require an explicit nested-entry audit before
implementing this manager-wide change; releasing lock inside current std_ops is
not an acceptable shortcut. DPC/IRQ/notify cessation is an additional drain domain,
not proved by active snapshot count zero.

## Necessary production changes and return contract

1. module.cpp init_module/get_module need an explicit internal retained-failure
outcome (not an arbitrary public negative status reinterpreted globally). Record
MODULE_ERROR/quarantine and a manager-owned pin with exact acquired-dependency
ledger BEFORE any failure unwind. get_module returns a negative status and does
not publish _info; it must not drop image/dependencies on retained failure. Clean
pre-init failure unwinds only acquired dependencies and returns original error.
2. get_module must check lifecycle/quarantine state before its positive-ref fast
path. A retained internal pin is not a successful user reference. put_module must
never consume that pin, panic on it, or report physical teardown success simply
because the external reference was consumed. Preserve current ABI if necessary
by adding an internal lifecycle result API: distinguish reference consumed,
teardown complete and quarantined; existing put_module B_OK is not a teardown
receipt. Failed uninit_module retains dependencies/image, with exactly one pin.
3. Add manager-owned split-phase lifecycle dispatch/serialization above, including
all relevant dependency/image release paths and nested recursive callers. Acquire
resources before exposing the callback; commit/release only on proven complete
return. Return Busy to conflicting acquisitions, negative quarantine on failed
lifecycle; do not reopen/reinitialize or automatically replay on uncertainty.
4. BusManager:177–299 must report stage/outcome to this internal contract: separate
pre-subsystem clean errors, partial INIT quarantine, later cleanup uncertainty,
drain timeout and fully clean teardown. Do not delete DPC on uncertain paths or
return successful UNINIT after failed termination/deletion. Preserve known owned
queue, callbacks, context, mappings and dependencies until reviewed recovery.
Route ACPICA lifecycle entries through the prior single-use permit checks, retaining
closure through all cleanup. Successful destruction alone publishes Closed.

These hooks are a necessary design, not existing support or sufficient proof of
all ACPICA callback/resource teardown. Exact internal API names/ABI, live thread
incarnation identity, broker exports, stage-aware unwinder and transitive callback
lock/ownership audit remain implementation prerequisites. No unavailable source
prerequisite was encountered for this bounded analysis. No standalone fixtures
would establish those guarantees; future tests must target actual manager/ACPI
production hooks and failure injection, including repeated get after quarantine,
partial dependency acquisition and DPC worker waiting on get/put.

Unsupported trusted-DMAR admission remains required; IntelMemoryOwner display
initialization, resident v0 dispatch and callbacks outside sLock are unchanged.
No production edits, builds, runtime tests, hardware operations, deployment,
replay or lock changes occurred. No GPU execution or DMA retirement is established.
Uncertain GPU-reachable backing remains owned. TERRA must independently reproduce
source identities/search coverage and check each matrix and interleaving claim
before ASTRA reviews this design.
