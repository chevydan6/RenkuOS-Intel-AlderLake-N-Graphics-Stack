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

## Astra source-verified allocation option (2026-11-07)

The existing private AGP.h allocate_memory callback already returns both an
aperture CPU address and a physical backing address. For the small first workload,
request B_APERTURE_NON_RESERVED | B_APERTURE_NEED_PHYSICAL and pass both outputs.
This is a candidate for reuse, not a newly implemented or physically validated
execution allocator. It avoids treating the aperture BAR's physical address as
system-page backing, and avoids borrowed bind_aperture entirely.

Verified in current agp_gart.cpp:

- AllocateMemory skips firmware/stolen backing only when placement is outside
  the reserved prefix. _Insert currently treats either NEED_PHYSICAL or
  NON_RESERVED as sufficient; request both explicitly and verify the range.
- NEED_PHYSICAL selects vm_page_allocate_page_run with WIRED and CLEAR flags.
  On 64-bit builds the current implementation restricts the run to below 4 GiB.
  vm_page.cpp defines high_address as an exclusive run-end bound. The first
  returned physical_page_number is exported only after successful binding.
- BindMemory uses memory->page + index to bind the same owned pages into GGTT.
  Successful outputs identify the CPU aperture alias and system physical base.
  GGTT offset is CPU alias minus aperture_info.base, with checked subtraction.
- DeleteMemory unbinds before freeing. Failed unbind retains metadata/pages.
  A failed allocation can still hide retained partial-bind records: the owner
  must disable publication and retain its lease as already specified above.

Use separate small allocations for context, ring, result/status and page-table
pages. A single page-table page needs no large contiguous pool. Reject returned
range overflow, non-page alignment, overlap with reserved backing, or an address
outside the established DMA mask. GART's alignment argument governs aperture
placement; it does NOT establish matching physical alignment beyond PAGE_SIZE.
Do not assume a 16 KiB context allocation implies 16 KiB physical alignment.
Store requested/rounded size, CPU alias, GGTT offset, physical base, owner lease
and mapping/publication state separately. No address or handle is a substitute
for its owner's lifetime. Free only through the same GART owner after quiescence.

Remaining pre-execution blockers:

1. intel_gart maps its CPU aperture with write combining and can fall back to
   default attributes. Cache type is not returned by AGP.h. A reviewed policy
   must establish publication ordering and result-read visibility for both, or
   explicitly reject an unsupported mapping. Do not create an unreviewed second
   CPU alias with potentially conflicting attributes.
2. intel_flush_tlbs currently reads the last GTT entry and executes wbinvd.
   This is not evidence of the required Gen12 engine TLB invalidation, ordered
   command publication, GPU cache flush, or completion-read protocol.
3. GART exposes system physical addresses, not an IOMMU DMA-mapping contract.
   Establish the target's DMA translation state; do not infer bus-address identity
   merely because the legacy display path uses physical page numbers.
4. Hardware page-table format, scratch mappings, permissions and stepping-specific
   invalidation remain required. PPGTT needs implementation even when allocation
   reuses GART. Physical addresses must never be inferred from GGTT offsets.
5. Shared aperture/module lease and failure quarantine still require reviewed
   implementation. This allocation option does not solve legacy borrowed R5.

Review request: Terra should check this smaller allocation option before a new VM
allocator is introduced. No production code changed for this proposal.

Source snapshot hashes (working source, not a claim that HEAD contains fixes):
- `headers/private/graphics/AGP.h`: `46daa69f810750ab1b4fcfd2d735e793a5d9f34cf060cadf64ec8b3c22f3513e`
- `src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp`: `ae4171b8ef0f200472dc9f15c104627b5a2e7c2d48da565a01f384fe644bcced`
- `src/add-ons/kernel/busses/agp_gart/intel_gart.cpp`: `d61ec6ec2e9e487f1444ee00bfe103b1ef2555908b89a0fa6293a304dba194d6`
VM allocator reference: source HEAD f01e0b782747076cb18d6c9f7952bcc358ee87e8,
`src/system/kernel/vm/vm_page.cpp`, vm_page_allocate_page_run.

## Concrete lease protocol for the next review

Terra's R3/R4 source/host review is accepted. Its T3 objection is also accepted:
the earlier operation names did not specify the atomic lifetime transition.
The following narrows the proposed implementation; it is not implemented yet.

Install the owner only on a fresh boot of the matching component set. There is
no hot adoption of an aperture created by an older legacy driver. In the new
N100 path, intel_extreme_init must acquire its display lease BEFORE the current
map_aperture call; the module performs that map once and returns the same
aperture alias/ID/info to the driver. Replace every N100 direct GART use in init,
allocation/free, error unwind and uninit with module forwarding. Other devices
retain their existing path. Existing userspace shared-info layout stays intact.
If the owner cannot acquire the backend because some other caller owns it, fail
initialization without unmapping/adopting that caller's aperture. No fallback
that leaves display outside the owner while enabling execution is permitted.

A module-owned device record contains state (Creating, Ready, Closing,
Quarantined), display lease count, request reference count, active operation
count, aperture ID/info and failure diagnostics. Device records and cookies have
module lifetime until explicitly destroyed after successful final cleanup;
never return a pointer to stack or legacy-driver state. Initial record creation
is serialized by a module registry mutex. A Creating record rejects concurrent
acquire with B_BUSY; map failure never publishes a Ready record.

Use the device mutex only for state/ref transitions. Before a forwarding GART
call: lock, validate the lease and admission state, increment active operations,
copy the aperture ID, unlock. The operation pin prevents final unmap. After GART
returns: lock, record success/error and any retained-ownership condition, decrement
active operations, unlock. Do not hold the device mutex across GART, worker wait,
user copy or MMIO polling. GART never calls back into this owner. Thus no new
owner/GART nested lock order is needed. A request acquires its reference under
the device mutex before its preparation operation can drop that mutex; worker
handoff retains that same reference before queue publication.

Final release under the mutex can select Closing only when display leases,
request references and active operations are all zero and display/GT quiescence
has been established. Closing prevents new acquisitions/operations. Its selected
cleanup worker owns the record while calling unmap outside the mutex. On success,
remove the record under registry serialization; on failure keep it Quarantined
with aperture ID and dependencies intact. New diagnostic work cannot reopen a
Quarantined record. An explicit reviewed recovery action, not a reference-count
zero event alone, is required to retry cleanup. Failed cleanup must never leave
a dangling cookie or a stale Ready record.

Display closure is not proof that scanout stopped. Until a verified display-stop
handoff exists, last-display-release retains the owner if quiescence is unknown;
it must not reclaim live framebuffer backing. Likewise a request timeout retains
its reference until retirement/reset quiescence, even after its client closes.
This is deliberate quarantine with recorded ownership, not silent success.

First fixed-store diagnostic uses bounded polling in a retained module worker;
no new GT IRQ handler is needed for that experiment if the reviewed hardware
completion protocol permits polling. Polling still requires CSB/status ordering,
retirement and timeout/reset handling. The worker's entry, helpers, captured
objects and register mapping must all belong to the module, never intel_info,
shared_info or callbacks in the unloadable display driver. The retained owner
must own its own MMIO area lifetime; sharing the driver's transient pointer is
forbidden. Exact MMIO mapping/alias policy remains a review item.

Required deterministic tests target each transition: close between pin and GART
call; close during failed rollback; acquire during Creating/Closing; worker
publication racing client death; concurrent final release; unmap failure followed
by rejected new work; stale lease use; unload of legacy image with retained
worker/MMIO/backing. No production implementation until Terra reviews this
protocol and resolves significant objections.
