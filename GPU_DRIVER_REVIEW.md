<!-- SPDX-License-Identifier: MIT -->
# Independent GPU memory-management review

2026-11-07. Terra review of Astra's uncommitted work, read-only production access.
Astra owns fixes and integration. No production file was edited by Terra.
This is a source/snapshot review, not project PR approval or hardware validation.

## Reproducible review baseline

Production root: `/home/dan/renkuos-development/source/gpu-next-worktree`.
Source is actively changing. The findings below refer to immutable copied bytes
in [evidence](research/terra-n100/evidence/) and
[SHA-256 manifest](research/terra-n100/evidence/renku-snapshot.json), not an assumed
clean HEAD. Paths/line numbers are relative to that production root and refer to
this snapshot; recheck live source before fixing. Existing build/test claims in
Astra's progress notes were read, not independently rerun or endorsed here.

## R1 — Critical, corrected in live source: nested manager lock

Location: `src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp:`418 (`get_aperture`),
line 1112 (`get_aperture_info`), and the outer manager guards in
allocate/free/reserve/unreserve/bind/unbind callbacks.

Each callback takes `sLock`, then calls `get_aperture`, which takes `sLock` again.
The module Jamfile selects `headers/private/graphics/intel_extreme/lock.h`:
`acquire_lock` increments count, and on a second acquisition calls acquire_sem
on an initially zero semaphore. The first guard cannot release until the nested
call returns. This is self-deadlock, even for missing IDs, blocking the global
GART service rather than merely creating slow contention.

Evidence: [independent reproducer](research/terra-n100/check_lock.py),
[recorded output](research/terra-n100/lock-check.log). It compiles the copied
production lock header, actual lookup helper and actual query callback. A minimal
OS semaphore stub detects the path that would block (exit 42); a temporary control
removing only the helper's redundant guard completes (exit 0). This is host
control-flow evidence; no native deadlock was induced.

Coverage gap: Astra's `gart-manager-lock-host.py:22` substitutes an unlocked
lookup helper, and `gart-threaded-host.py` also substitutes the lookup helper.
Those suites cannot detect the production double acquisition despite testing
outer lifetime protection. Native fixture compilation does not establish that
a real graphics client exercises this production callback on the target.

Recommendation: retain outer lifetime guard and use an explicitly caller-locked
lookup (or direct hash lookup), auditing every caller. Include the actual helper
and selected lock implementation in the regression. Do not solve this by removing
lifetime protection or silently substituting recursive locks.

## R2 — High, corrected in live source: unchecked hash publication

Locations: `src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp:`523
(`CreateMemory`) and line 1069 (`map_aperture`).
Supporting definition: `headers/private/util/OpenHashTable.h:170` (`Insert`).

Both Insert results are ignored. This hash implementation starts with no bucket
array and can return B_NO_MEMORY on its first insertion. In map_aperture the code
still publishes base and returns an ID, but future lookup cannot find the object:
backend singleton, module reference and aperture are orphaned. In CreateMemory,
_Insert has already linked the record into fFirstMemory; failure to hash it still
returns success to allocation/bind, leaving free_memory unable to find it. A later
Remove on a zero-sized hash also assumes storage exists and is unsafe.

Recommendation: make metadata publication transactional. Check insertion before
publishing handles/output, unlink and destroy on failure, and retain/recover any
already live backing explicitly. Test *initial bucket allocation*, not just the
object/page allocation. This is source-verified error-path analysis; no allocator
fault was injected into the native kernel by Terra.

## R3 — Medium: aperture and manager semaphores are not destroyed

Locations: `src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp:`447
and line 1345; creation at constructor/init.
`headers/private/graphics/intel_extreme/lock.h` requires `uninit_lock` to delete
the semaphore. The aperture destructor only deletes backend and releases module;
agp_uninit does not destroy sLock. Repeated successful aperture open/close leaks
one semaphore per aperture, and manager reload adds another. The hash object's
allocation lifecycle also merits explicit module teardown review.

Recommendation: destroy initialized locks after final use, and balance initialization
failure paths. Verify sem/module/allocation counts after repeated open/close and
forced constructor failures. This appears inherited, but affects the current
recovery/reopen workflow and is not excused by its age.

## R4 — Medium: shrinking hash removal during teardown invalidates iterator

Location: `src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp:`1345.
The loop obtains a hash iterator, calls regular Remove, and continues iterating.
`headers/private/util/OpenHashTable.h:203` can shrink/rebuild the bucket table;
its comment explicitly prohibits regular Remove during iteration and specifies
RemoveUnchecked. With a table expanded beyond minimum size, teardown can miss or
revisit entries after shrink. The same issue was already addressed in the new
map_aperture recovery scan, but remains in module uninitialization.

Recommendation: use supported iteration-safe removal and check a multi-aperture
expanded table, including failure midway. Intel singleton constraints reduce the
single-device trigger; this manager is generic and supports other backends.
This is a source-verified conditional defect, not a reproduced N100 failure.

## R5 — High integration gap: external backing lifetime after failed bind

Locations: `src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp:`684
and line 1226.

The external bind path resolves CPU backing with get_memory_map; it does not own
or pin the external area/pages. Partial successful bind followed by failed
rollback retains the GART record, but the caller receives only an error and no
new base. The retained record tracks bound_size, not an area reference/pin or
caller recovery token. If the caller releases external backing after failed
bind, retained GGTT entries may reference reclaimed pages. Current Intel backend
normally fails by validation rather than an asynchronous hardware error; the
conditional generic callback failure path is nevertheless not a safe contract
for future execution buffers.

Recommendation: audit and document every caller's pin/lifetime behavior, including
error returns; provide owned backing and cleanup state/recovery ownership before
using this service for DMA. Test failure after page N, failed rollback and caller
area deletion. This is a conditional lifetime concern requiring caller review,
not a claim that every current caller releases its area unsafely.

## Hardware compatibility and other reviewed limits

- `src/add-ons/kernel/busses/agp_gart/intel_gart.cpp:1026` flush_tlbs performs
  PTE readback and wbinvd. That is not the Gen12 engine TLB invalidation protocol
  demonstrated in Linux intel_tlb.c. Existing GGTT/display success cannot validate
  PPGTT changes or GPU request cache visibility. Treat this as a submission gap,
  not a claim that build 057 display mappings failed.
- Prefix tracking/unbind-from-tail preserves backing on failure; this is safer
  than freeing pages whose PTEs may remain. It does not establish GPU quiescence.
- Review cleanup-only records before new allocation or bind. Live source acquired
  IsCleanupPending guards after the snapshot; do not report their absence as an
  unresolved new defect without inspecting that update.
- GART module reference retention is distinct from pinning Intel driver code,
  client memory or future in-flight contexts. Unload retention remains unresolved.
- No incorrect cross-thread lock order was independently reproduced beyond R1;
  no claim of race-free operation follows from this limited review.

## Required tests and disposition

Run [the test matrix](GPU_FIRST_EXECUTION_PLAN.md#validation-specifications),
prioritizing R1 production-helper regression, hash initial-allocation failure,
semaphore accounting and expanded-table teardown. R1 host reproduction was run;
other cases are specified, not claimed executed. No physical GPU execution,
new memory hardware validation or native kernel stress was performed by Terra.

Astra accepted R1 and corrected the helper after the original snapshot. Terra
independently compiled the live helper/header/query in check_lock.py --expect-fixed:
exit 0. A copied version of Astra's updated actual-helper threaded fixture passed
100 ASAN/UBSAN rounds against the captured corrected manager. All seven callers
retain the outer guard. The same actual-helper threaded fixture rejects the old
snapshot within its bounded gate. Evidence: [correction manifest](research/terra-n100/evidence/correction-validation.json),
[threaded pass](research/terra-n100/threaded-correction.log),
[negative control](research/terra-n100/threaded-negative-control.log),
[single-acquisition pass](research/terra-n100/lock-fixed-check.log).
This closes R1's source/host regression; native/kernel and physical validation
remain Astra's integration gates. Other callbacks and backend behavior remain mocked.

Astra accepted R2 and supplied patch 0076. Terra independently inspected both
corrected publication paths: CreateMemory unlinks/deletes on failed Insert before
allocating backing; map_aperture inserts before output publication and destroys
unpublished aperture/backend/module ownership on failure.

Terra's [real-hash fixture](research/terra-n100/check_hash_publication.py) compiles
the actual OpenHashTable header with a faulting bucket allocator and production
CreateMemory/_Remove. It passes 100 initial-bucket failures, list/record accounting,
retry and cleanup under ASAN/UBSAN; the old snapshot fails the negative control.
The placement helper and OS services remain modeled. Terra also independently ran
the captured updated map/unmap recovery fixture: 18 cases pass, including unchanged
output on registry failure; that fixture injects Insert's return rather than real
registry bucket exhaustion. [Hash result](research/terra-n100/hash-publication.log),
[negative control](research/terra-n100/hash-negative-control.log),
[recovery result](research/terra-n100/r2-map-recovery.log),
[correction hashes](research/terra-n100/evidence/r2-validation.json).

This closes R2's reviewed source/host publication failure path. Actual kernel
allocator exhaustion and physical memory validation were not run by Terra.
Astra reports native build 074 validation; this report does not independently
claim those native results. R3/R4 fixes and R5 lifetime design remain open.
Complete caller/DMA lifetime design before submission.


## A-F1 independent design check: retained execution module

Astra's proposed mechanism is supported by source at RenkuOS commit
`f01e0b782747076cb18d6c9f7952bcc358ee87e8`. `src/system/kernel/module.cpp:2216`
adds a permanent reference only after successful B_KEEP_LOADED initialization;
put_module at line 2262 guards against dropping that retained module to zero.
`src/system/kernel/device_manager/legacy_drivers.cpp:470` ignores uninit_driver's
return and unloads the legacy image at line 476. Therefore returning an error from
legacy uninitialization cannot retain in-flight execution code/state.
[Copied sources and hashes](research/terra-n100/evidence/module-review.json).

Recommendation: accept a retained module as a candidate execution-code owner.
Its request records, workers, IRQ callbacks and teardown functions must all remain
in retained code, with no function/data pointers into the unloading legacy image.
This alone does not pin external pages or stop display close from unmapping a
shared aperture. Require explicit aperture leases/reference ownership, disallow
teardown while leased/in-flight/quarantined, and test failed module initialization,
client exit, driver unload and reset failure. Integration design remains pending;
this is not approval of a not-yet-written execution service.
