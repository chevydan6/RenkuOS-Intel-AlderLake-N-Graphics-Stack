<!-- SPDX-License-Identifier: MIT -->
# Execution ownership proposal for Terra review

2026-11-07. Proposed interfaces, not implemented functionality. This narrows the
first experiment's resource contract without claiming that borrowed GART binding
is repaired. It requires review before production implementation.

## Scope and supported lifetime

The fixed kernel-built direct-ring MI-store workload accepts a nonce and returns
structured evidence. It accepts no user address, area ID, physical address, GGTT
offset or external batch. All GPU-visible resources must be kernel owned. Do not
call the borrowed bind_aperture path for execution resources. This excludes R5's
unsafe borrowed-backing contract from the first workload; R5 remains a real
legacy API limitation and a prerequisite for any future external-buffer import.

Use a private retained kernel module for the execution owner. Verified RenkuOS
get_module gives B_KEEP_LOADED modules a permanent reference after successful
initialization. Legacy unload_driver does not honor an uninit veto. Therefore all
worker code, IRQ callbacks, request metadata, recovery and quarantine must reside
in the retained module, with no function/data pointers into intel_extreme's
unloadable driver image. Retention is not a substitute for successful cleanup:
normal request completion must still release resources after quiescence.

## One aperture owner shared with display

Do not create a second Intel backend aperture or merely remember the driver's
aperture ID. Introduce one device-level owner of the existing GART aperture; the
display driver holds a lease and the diagnostic request holds a separate lease.
This is required because driver uninit currently calls unmap_aperture, which can
delete every allocation even if another component retained an ID or module ref.

Proposed private module operations: acquire_device(PCI identity), release_device,
run_fixed_store(nonce, result), query_diagnostics. They are design names, not
existing RenkuOS APIs. Acquire returns an opaque kernel cookie, never a user
handle or CPU address. Display forwarding operations retain existing legacy
allocation semantics and trusted accelerant ABI; separate display allocations
from request-owned allocations in the owner's ledger. Do not impose per-open
ownership on cloned accelerants.

The owner maps the aperture once. Display release cannot unmap while a request,
recovery or quarantine reference exists. Request release cannot free display
backing. Final unmap requires zero display leases, zero in-flight/recovering
requests, and verified GT quiescence. If unmap fails, retain the device record,
GART ID and dependencies. No blind reopen/adoption of old allocations.

This integration requires careful review of display-driver init/uninit and
failure unwind; do not substitute it for preserving build 057 hardware behavior.
Only the lifetime boundary changes; modeset programming stays untouched.

## Allocation and partial-binding failures

Prefer owned allocations with a complete DMA-page ledger for context, ring,
result/status and page-table backing. Existing GART owned allocation retains
pages when rollback fails, but hides the allocation address on failure. The
owner must not invent a retry handle or free pages independently.

Until a reviewed transaction interface exposes that state, the fixed diagnostic
must treat any resource-allocation failure as non-publishable. Roll back known
objects through GART; keep the aperture owner and module alive. Mark diagnostic
execution disabled for that owner when cleanup cannot be established. Do not
retry allocations indefinitely or reclaim the whole aperture while display
leases exist. A later exclusive teardown/recreation after display and GT stop
can recover hidden GART records. Report that limited recovery explicitly; it is
not an application-safe managed-buffer API.

Physical page-table backing also needs an exact owned-page/DMA mapping contract;
GART aperture CPU addresses are insufficient for building PPGTT entries. Before
choosing an allocator, verify the RenkuOS VM ownership APIs and 39-bit DMA reach.
CPU aliases, cache attributes, pinning and device invalidation remain mandatory.

## Locking and publication

Device state mutex protects lease/reference counts and the request state machine.
Never hold it across user copy or GPU completion waits. Establish and test its
order relative to GART sLock before nesting calls; no callback into the owner
while holding GART locks. Workers take request references before dropping locks.

Prepare all metadata and mappings before publication. Once published, only
matching sequence/reset-generation retirement or proven reset quiescence permits
release. Timeout/client death means retained ownership, not cancellation by free.
Reset failure wedges diagnostic execution and retains the full DMA object graph.

## Review acceptance tests

1. Display closes while a diagnostic request is blocked in completion: aperture,
   pages and worker code remain valid; display records are not reused by request.
2. Diagnostic client dies before/after publication: unpublished state rolls back;
   published state remains kernel-owned until retirement.
3. Each allocation/bind stage fails, including rollback failure: no publication,
   no lost owner, no premature free, no unbounded automatic retry.
4. Unload legacy graphics image while owner retains state: no callbacks or data
   references into unloaded image; no backend aperture destruction.
5. Failure then explicit final teardown/recreate: hidden retained GART records
   are reclaimed only after every lease and GPU use is quiescent.
6. Concurrent close/query/reset and stale completion: exactly-once retirement,
   balanced refs, no allocator reuse before idle.
7. Physical memory-only validation first: matched components and verified
   recovery medium, HDMI preserved, logs captured; no command publication yet.

Astra decision: request Terra review of this proposal before creating a retained
module or changing display ownership. The immediate next implementation should
be the smallest coherent device lease and owned-resource path with tests, not an
unused generic object registry or a full Mesa/DRM port.
