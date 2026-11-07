# Technical findings

Terra appends numbered findings with source/function, evidence, severity and action. Astra appends disposition without editing Terra's evidence. Major design decisions require Terra review before implementation.

## Astra handoff A-F1: module lifetime (awaiting Terra review)
At source f01e0b78, module.cpp get_module grants a permanent reference after successful B_KEEP_LOADED initialization. put_module cannot normally reduce that retained module to zero. legacy_drivers.cpp unload_driver ignores uninit_driver's result and unloads the image. A retained execution module is therefore a supported candidate owner; pointers into legacy driver code/state cannot survive its unload. A shared aperture must not be freed by display close while execution retains mappings. No owner module is implemented yet.

## Terra T-F1 — minimum execution path (2026-11-07)

Verified Linux pin 028ef9c96e96197026887c0f092424679298aae8:
ADL-N inherits ADL-P Gen12 logical contexts/ELSQ. Linux LRC selftest issues MI
STORE_DWORD_IMM directly in a request ring. Proposed simplification to Astra's
separate-batch design: one BCS0 trusted direct-ring nonce/complement store to
owned GGTT result, then Gen12 ordered flush/breadcrumb and context retirement.
No separate batch buffer is necessary for the earliest command-streamer proof;
valid context, root/scratch page tables, power/cache/TLB and reset remain needed.
Astra design disposition requested; no driver implementation by Terra.

## Terra T-F2 — firmware and cache-policy evidence

Linux policy defaults differ from hard firmware requirements. Execlists remains
an alternative when GuC submission is disabled. ADL-N GuC selection redirects to
ADL-S/TGL family due to absent HWConfig; do not infer adlp firmware from device
info. No physical firmware-free claim is established.
MOCS retrieval gap resolved: get_mocs_settings selects gen12_mocs_table for ADL-P.
Complete PAT/stepping/visibility review is still a prepublication requirement.

## Terra T-F3 — independent check of Astra A-F1

Verified RenkuOS f01e0b782747076cb18d6c9f7952bcc358ee87e8:
module.cpp:2216 adds permanent ref on successful B_KEEP_LOADED init; legacy_drivers.cpp:470
ignores uninit result and unloads image. Candidate retained execution module is
supported, but all workers/callbacks/data must live in retained code. It does not
pin external backing or retain shared aperture ownership automatically. Require
explicit aperture lease ownership and unload/client-death/reset-failure tests.

## Terra deliverables and test specifications

Research branch research/terra-n100-first-execution, worktree /home/dan/renkuos-terra-n100:
INTEL_N100_GPU_ARCHITECTURE.md; GPU_COMMAND_SUBMISSION_RESEARCH.md;
GPU_FIRST_EXECUTION_PLAN.md; GPU_DRIVER_REVIEW.md.
First-execution plan contains host/native/physical-separated test matrix for
concurrency, allocation failure, cleanup, initialization failure, timeouts,
stale completions and hardware-error recovery. Packet/context fields are source
verified candidates, not an approved complete hardware initialization recipe.

## Terra T-F4 — further initialization specifics and delivery

Source lookup confirmed BCS0 base 0x22000, ELSQ 0x22510/control 0x22550;
copy context base image 8 KiB plus two Gen12 workaround pages (16 KiB before
optional debug redzone); GT forcewake range for BCS registers; BCS TLB invalidation
0xcee4 bit 0 and Wa_2207587034 additional invalidate. Exact pinned function/line
links in GPU_COMMAND_SUBMISSION_RESEARCH.md. These are source facts, not a complete
hardware-ready initialization recipe or live stepping verification.

T1 initial review and T2 research/specification deliverables complete for this
checkpoint. R1 and R2 source/host fixes independently validated; native/physical
validation and ownership design remain open. Astra owns task-board transitions.

Delivery: /home/dan/renkuos-terra-n100/research/terra-n100/delivery.patch plus
evidence-sources.tar.gz and SHA-256. Patch dry-run applies cleanly. Shared Git
object database is not writable by this agent, so no commit was attempted after
the add failure and no permissions were changed. Four requested reports remain
in the separate worktree. Source archive restores copied/hash-pinned evidence.


## Astra disposition of Terra research and ownership review request

T-F1 accepted: direct-ring nonce/complement stores replace separate batch for
first proof. T-F2 accepted: MOCS source gap resolved; stepping/cache review stays
open. T-F3 accepted: retained module is supported but needs explicit backing and
aperture ownership. T-F4 accepted as pinned source facts, not hardware validation.
Reviewed delivery integrated as cd65120, including verified evidence archive.
R1/R2 independent host scripts rerun successfully against current production.

Terra T3 request: review research/n100-submission/EXECUTION-OWNERSHIP.md.
Key questions: one shared aperture owner with display/request leases; retained
module owns all execution callbacks/data; fixed diagnostic admits no borrowed
areas; failed allocation disables publication and retains owner until safe
exclusive cleanup. No module or display lifetime code changed yet. R5 remains
open for borrowed API callers. Provide design objections before implementation.

## Astra A-F2 — reuse existing owned physical GART allocation

Source inspection confirms NON_RESERVED | NEED_PHYSICAL returns owned wired,
cleared contiguous backing below 4 GiB and its physical base after binding.
Candidate for small first-workload objects; avoids adding a separate allocator.
Detailed source hashes, address distinctions and restrictions appended to
research/n100-submission/EXECUTION-OWNERSHIP.md. Physical alignment beyond page
size, CPU mapping cache policy, Gen12 TLB/cache ordering, IOMMU state and shared
owner lifetime are NOT established by this API. No execution implementation or
physical test. Terra review requested before selecting this path.

## Terra review of A-F2 — owned GART pages (2026-11-07)

Disposition: acceptable candidate for the first small, kernel-owned execution allocations, subject to the existing ownership and hardware gates; not yet an execution-ready allocator. Independently inspected agp_gart.cpp:559–595, 693–765, 840–855 and 1134–1194. NEED_PHYSICAL chooses a cleared wired contiguous page run; successful bind uses those pages and only then returns physical base and aperture alias. Existing 64-bit path constrains allocation below 4 GiB. Alignment governs GGTT placement, not stronger physical alignment. Preserve distinct physical, DMA, GGTT-offset and CPU-address fields; validate the entire rounded range, not just its first page.

Clarification: current _Insert checks (B_APERTURE_NON_RESERVED | B_APERTURE_NEED_PHYSICAL), so NEED_PHYSICAL itself already selects placement after the reserved prefix in this source. Keeping NON_RESERVED explicit is sensible documentation and defense, but it is not independently mandatory for this exact implementation. Verify the returned range regardless, and test flags separately.

Blocking before hardware publication: establish whether physical equals device DMA address under the actual IOMMU configuration; full-range DMA-mask and per-object alignment; WC/default mapping detection or a proven policy covering both; CPU publication ordering and completion-read visibility; Gen12 GPU TLB/cache invalidation; safe lifetime of page-table and scratch backing through reset/quarantine; shared aperture leases; partial-bind failed-cleanup quarantine with no guessed handle. Page-table writes through the aperture alias and GPU page-table walker reads require an explicitly verified coherence contract, not just successful GGTT mapping. The GART flush callback is not proof of these properties.

Suggested host tests: check both flag combinations exclude reserved range, page-rounding and physical/GGTT range overflow, failed bind/rollback with unchanged outputs and retained owner, and distinct address types. Hardware tests must separately establish mapping attributes and translation/coherence; mocked tests cannot establish these. No hardware operation or production modification by Terra.

## Terra T3 review — concrete aperture lease protocol (2026-11-07)

Disposition: sound direction for a reviewed, staged implementation; not yet approval for GPU publication or physical deployment. Cold-boot conversion, operation pins, Closing admission, retained worker/MMIO and no cleanup without display/GT quiescence address prior objections. Existing device.cpp:248–261 calls intel_extreme_uninit on last close under gLock; conversion must cover every GART path including get_aperture_info, initialization failures, and last-close unwind, while preserving non-N100 behavior.

Implementation blockers requiring explicit resolution:

1. Allocation failure with hidden rollback failure must transition to a publication-blocked/quarantined state under the device mutex *before* dropping the final operation pin. Other in-flight preparations and already admitted requests must be serialized or recheck state at their actual publish point. A request already published must retain its complete DMA graph until independently retired or reset. Define whether all allocation errors conservatively quarantine or how cleanup failure is distinguished; current callback may return cleanup status without an ownership token.
2. Define registry/device mutex order, record reference acquisition under registry lock and removal/free after the last cookie/operation/worker reference. A stale cookie must not dereference freed/reused memory; generation-tagged handles or retained tombstones are possible. Closing, final removal, concurrent acquire, worker handoff and quarantine recovery need deterministic ABA/race tests.
3. Specify display lease reopening when last close leaves scanout active and the owner retained: whether reacquisition reuses existing Ready record without reinitializing GGTT or rejects, and how the driver's init failure/unwind avoids destroying retained scanout. Cold-boot-only conversion prevents adoption of an older owner, but does not resolve reopens in the same boot. No scanout mapping or state can be reclaimed merely because the legacy image unloads.
4. Module-owned MMIO mapping and backend/module references must survive display-driver uninit; prove no driver-owned PCI/info/area pointer crosses the boundary and resolve MMIO alias/cache attributes. Registry retention and B_KEEP_LOADED alone do not protect those resources.
5. Recovery of Quarantined objects must define exclusive ownership and a verified transition to idle; explicit retry cannot race active pins or a retained scanout. Preserve original state if unmap fails.

Add deterministic tests for failure after another operation has been admitted but before publication, stale cookie after successful final cleanup/recreation, close/reopen with scanout active, initial display-init failure, and two simultaneous final releases. Keep existing R5 borrowed-memory limitation, DMA/IOMMU, cache and TLB gates. No physical test; reported SSH no-route means current remote recovery is unverified.

## Terra T3 disposition — memory-only implementation readiness (2026-11-07)

The revised permanent-record/numeric-lease protocol resolves the five previous design objections sufficiently for Astra to begin an isolated memory-only implementation and tests. This is design disposition, not implementation approval, physical admission or GPU execution authorization. RetainedDisplay rejects same-boot display reopen: document and exercise this explicit usability limitation, including init failure and last-close paths; preserve the known-working boot option before any eventual hardware installation.

Implementation conditions: numeric IDs must be scoped to the boot/device and validated under the same mutex as revocation; acquire allocates/publishes entries transactionally, exhaustion fails closed. All N100 GART and MMIO area ownership paths, including shared-info clones, failure unwinds, client death and device_free, must actually route through the retained owner, with no legacy pointers. Every failed owned allocation blocks diagnostic admission before unpin, while preserving valid display cleanup operations and existing allocations. Preserve permanent record and module dependencies on every failed teardown. Write deterministic race/negative-control tests for the transitions listed in the design; use mocked tests to validate state logic, then native kernel tests before any physical smoke.

Future publication gate: checking/reserving under the mutex alone does not automatically order the later MMIO submission against a concurrent display allocation failure after unlock. Before implementing GPU publication, define a publishing state/pin and admission serialization such that no allocation failure can race between the final state check and the actual hardware publish, or explicitly establish a different safe contract for an already-reserved request and its retained resources. Test that exact interleaving. This is not a blocker for an implementation that has no GPU publication path.

The revised protocol supersedes earlier language about freeing/recreating device records and in-place quarantine retry. IOMMU/DMA address identity, mapping cache attributes, engine invalidation, reset and scanout quiescence remain unresolved hardware gates. No independent hardware access; SSH recovery remains unverified.

## Terra native owner lifecycle validation specification

Prepared /home/dan/renkuos-terra-n100/NATIVE_OWNER_VALIDATION_SPEC.md for Astra's 0084 candidate. Covers native module/dependency faults, pin/release, RAM-backed cloneable area and team death, synthetic IRQ drain/MSI ordering, init/close unwind, normal desktop open/clone sequence and explicit RetainedDisplay reopen rejection. Distinguishes the actual VM display baseline from a simulated N100 backend and physical GPU evidence. No tests or hardware operations performed by Terra. Build 084 pass is Astra-reported, not independently rerun.

## Terra GT execution follow-up

Added /home/dan/renkuos-terra-n100/research/terra-n100/FORCEWAKE_CSB_FINDINGS.md with pinned Linux references: forcewake masked writes, ACK/fallback behavior and failure cases; Gen11+ 12-entry CSB initialization, stale-sentinel handling and Gen12 event parsing. These are source findings and simulation test specifications, not a hardware-ready implementation or permission to execute commands.

## Terra native kernel owner probe: source ready for Astra review

Source-only test in /home/dan/renkuos-terra-n100/research/terra-n100/native-owner/kernel-probe/: terra_owner_probe.cpp (SHA256 ca651ee0bc6c0a5efbeaa3b9360e3fb86e0d67720020715e373b7871451bde9c), client.cpp (e89fedd19d8eafdd1bea1fc990adcdcf2aa92609a6ae567c27e638ad652b6bdb), protocol.h, README.md. Legacy test driver creates per-open wired, cloneable RAM kernel area, calls actual get_module/put_module on retained owner and asks it to reject impossible PCI 255:255.255 with unchanged outputs. User client clones RAM area, validates both-direction sentinel, closes test driver and verifies clone backing remains. No forced N100 match, GPU BAR mapping, MMIO or hardware workload.

Not compiled or executed; review SDK and driver installation details first. Successful real owner module acquisition permanently retains module/dependencies, so disposable VM only. Astra owns build, installation, VM execution and artifact capture; use canonical locks and watchdog. The RAM clone is a surrogate, not real MMIO/IRQ or hardware display validation. Earlier userland cross-team positive and negative RAM controls independently passed native VM under locks; details in HARDWARE_TEST_RESULTS.md.

## Terra Vulkan mission started

User assigned Terra Vulkan userspace research, Lavapipe and ANV; Astra retains kernel ownership. Independent repository/worktree: /home/dan/renkuos-terra-vulkan (research/vulkan-userspace). A disposable VM baseline probe is running under canonical locks, no passthrough or physical deployment. Kernel/userspace contract will be proposed for Astra review before implementation; no existing kernel ABI is presumed.

## Terra Vulkan requirements for Astra — review requested

Initial requirements: /home/dan/renkuos-terra-vulkan/GPU_VULKAN_INTERFACE.md. This is a proposal, not an agreed ABI. It identifies versioned discovery/capabilities, per-client managed BOs, mapping/coherence, VM binding/invalidation, contexts/submission, fence/timeline completion, device-loss/reset and optional sharing/WSI. Mesa ANV also directly depends on DRM discovery and syncobj outside its KMD backend; a fixed kernel MI-store diagnostic alone is not an ANV interface. Please review requirements when driver work permits; no kernel design changes requested before first verified execution.

Software path: host-only CPU Vulkan image clear/readback passed, no RenkuOS Vulkan workload yet. Disposable native VM lacks vulkan headers and the checked ICD directories; package/library inventory inconclusive due missing rg. Corrected inventory awaits canonical host-config lock (Astra VM active). No hardware deployment.

## Terra Vulkan native software milestone

Disposable native VM hrev60072+518: HaikuPorts loader vulkan-1.4.311, vulkan_devel and mesa_lavapipe-22.0.5 activated; existing llvm12_libs used. Loader trace selected libvulkan_lvp.so, CPU llvmpipe LLVM12 reported Vulkan 1.2.204; offscreen 32x32 RGBA8 clear/copy/readback verified all 1024 pixels. Native compiler GCC13.3, VM shut down cleanly. Evidence /home/dan/renkuos-terra-vulkan/evidence/vm-software-002/ and VULKAN_BASELINE.md. This proves headless software Vulkan in this VM only, no GPU execution or desktop presentation. Mesa warns Lavapipe is not conformant. First package attempt failed on duplicate LLVM activation; clean overlay retry passed.

Kernel/Vulkan requirements remain a draft at /home/dan/renkuos-terra-vulkan/GPU_VULKAN_INTERFACE.md for Astra review. No i915/DRM ABI or kernel implementation was assumed. Next Terra work: headless shader/compute test and ANV adaptation research, independent of Astra kernel work.

Terra Vulkan follow-up: in a second isolated native VM using the successful Lavapipe overlay, a Vulkan 1.0 SPIR-V compute shader executed on CPU llvmpipe and returned 256 verified `3*i+7` values. Loader trace identifies libvulkan_lvp.so; VM shut down cleanly. Evidence /home/dan/renkuos-terra-vulkan/evidence/vm-compute-001/. This is software Vulkan only, not Intel GPU execution.

Terra software Vulkan graphics: native disposable VM with loader1.4.311/Lavapipe22.0.5 rendered offscreen full-screen triangle using vertex+fragment SPIR-V and verified 1024 green pixels; clean shutdown. Native instance enumeration returned nine extensions, without VK_KHR_surface or VK_EXT_headless_surface for this exact environment. Evidence /home/dan/renkuos-terra-vulkan/evidence/vm-draw-001/ and VULKAN_BASELINE.md. Host no-draw control fails expected; native no-draw control not run. This is CPU software only, not physical GPU or desktop presentation.

## Terra preliminary native synthetic IRQ probe assessment

Pinned kernel interrupts.cpp:638–687 allocates software vector metadata; with INTERRUPT_TYPE_UNKNOWN and B_NO_ENABLE_COUNTER, install skips IRQ-only CPU assignment and enable-counter arch_int_enable, so the inspected generic path does not intentionally route a physical device interrupt. This is not yet proof of zero architecture/debug side effects: install_io_interrupt_handler calls arch_debug_remove_interrupt_handler(vector) unconditionally; io_interrupt_handler updates interrupt-load/statistics and debug builds can disable a vector after repeated unhandled events. The private allocator, free and dispatch symbols must be checked for actual `_KERNEL_` linkage on the target SDK before writing a runnable module; no export guarantee follows from private headers.

Important deadlock blocker in the proposed SMP barrier: callback runs with interrupts disabled while holding vector spinlock. A remover spins on the same lock with interrupts disabled. On a two-vCPU guest those two threads can occupy both CPUs and prevent the controller that releases the callback from running. A sleep/semaphore in the callback is invalid. A native stress design needs at least three independently schedulable CPUs (prefer four) and verified affinity, a bounded non-sleeping callback spin with atomic flags, a third controller that releases before its deadline, an outer watchdog/QMP recovery, and a fail-safe bounded callback timeout. Verify current processor/affinity API before trusting placement. A host-thread model is not proof native interrupt spinlocks are safe.

Allocator source also initializes `runFound=true` and `vector=0`; if every vector is already allocated, the for-loop can exit without setting runFound false and may incorrectly return an already allocated vector. Do not assume successful allocation alone proves exclusivity under saturation; this is a source-level potential bug requiring separate reproduction and kernel-owner review. No allocator stress, kernel module, VM IRQ experiment, or production changes were performed by Terra. Until the allocator edge and symbol exports/architecture effects are resolved, recommend not running the suggested IRQ experiment.

## Astra: interrupt allocator exhaustion reproduced

Accepted Terra source finding; actual extracted allocator fails255/2048 exhaustive
host occupancy/request cases. Full eight-slot occupancy plus count1 wrongly returns
B_OK at already-owned vector0. Isolated one-line runFound=false candidate passes
2048/2048 and preserves outputs/storage on failure. Host compiled and executed;
no native kernel compilation/test or physical operation. GPU source unchanged.
Evidence testing/irq-vector-001/ and research/n100-submission/interrupt-vector-exhaustion.txt.
Candidate is outside production patches/, not installed. Independent review requested.
This is a direct x86 MSI allocation dependency, required for IRQ ownership proof;
full kernel build/provenance/native boot validation is separate from GPU component
builds. Synthetic IRQ stress remains held for allocator, export/architecture and
bounded SMP scheduling requirements. No claim current runtime kernel matches source.

### Terra: upstream Vulkan application in native VM (2026-11-07)

Unmodified Sascha Willems `renderheadless` at commit `6e1cc299c22eaf2b1771944b3adb7b8258f84ff9` compiled in the disposable RenkuOS/Haiku VM and ran with Lavapipe; the loader identifies llvmpipe. Independent PPM inspection found 1024x1024 pixels and 92,123 colors (SHA-256 `5a66a675b527ec43ef9f96eef127f76677aff29549dbd297659427dd2d24c75e`). The sample was compiled with `-DNDEBUG`, so assertions were disabled; no reference-image comparison was performed. This is software execution, not Intel GPU execution or desktop presentation. Evidence and exact dependencies: `/home/dan/renkuos-terra-vulkan/evidence/vm-upstream-001/` and `VULKAN_BASELINE.md`. VM shut down cleanly.

### Terra: disposable candidate kernel boot lookup

Read-only loader audit: `loader.cpp:28-87,110-130` searches `system/kernel_x86_64` through the boot system directory, while `vfs.cpp:520-577` mounts the selected system package as that directory. The non-packaged paths in `loader.cpp:48-54` are for add-ons, not the kernel. A non-packaged kernel override is therefore not established. Candidate package replacement on a disposable overlay is a possible route, with package activation, compatibility, and runtime identity still to be proven. Full findings: `/home/dan/renkuos-terra-vulkan/KERNEL_BOOT_RESEARCH.md`. No boot or deployment performed.

### Terra: retrace semaphore lifetime and user ABI

The owner team can exit/exec while cloned clients still hold the published semaphore ID; owner cleanup deletes it and wakes waiters with B_BAD_SEM_ID. Kernel ownership alone breaks userspace acquisition, and app_server caches the publicly exposed retrace ID. Full ID generation checks prevent immediate alias on slot reuse; eventual wrap is a theoretical stale-ID concern, not demonstrated UAF. A designated user owner is only viable with a proven lifetime/handoff and client refresh protocol; no existing independent-lifetime user-accessible semaphore primitive was identified. Kernel-mediated lifetime or per-client proxy alternatives require design. Detailed source references and test specifications: `/home/dan/renkuos-terra-vulkan/RETRACE_SEMAPHORE_RESEARCH.md`. Astra notified.

### Terra: production IRQ retention boundary

No-lock dispatch can traverse a handler node concurrently with removal/free; retaining callback data only after removal does not protect the node. A general drain needs dispatch-side admission before list access, or an equivalent proven lifetime mechanism. Interim retention must include registration, vector/MSI, callback image and data, MMIO/owner lease, shared state and semaphore, and block reinit. Legacy framework can unload driver text despite uninit failure; callback may re-enable hardware interrupts after teardown masks. Source-level risk, production vector reachability unverified. Full analysis: `/home/dan/renkuos-terra-vulkan/IRQ_LIFETIME_BOUNDARY.md`. Astra notified.

### Terra: opt-in locked-vector proposal

Permanent locked-vector latch is a plausible narrow callback-drain contract, conditional on atomic registration and conflicting no-lock rejection before publication, explicit old-kernel capability negotiation (unknown flags can be ignored), race-free publication and audit of permanent reuse/allocator/internal-vector impacts. Existing arch debugger removal side effect precedes the vector lock and needs attention on rejection. It does not solve post-dispatch accounting, MSI reuse or hardware mask/re-enable sequencing. Detailed gates: `/home/dan/renkuos-terra-vulkan/IRQ_LIFETIME_BOUNDARY.md`. Design only; no kernel changes or runtime validation.

### Terra: retained IRQ trampoline proposal

Retained owner trampoline with IRQ-safe callback lock can drain borrowed driver callback/text/data, but retaining its own node does not protect other shared handler nodes from no-lock traversal/removal races. Require proven exclusive MSI/no other registrations or kernel list-lifetime contract; fail closed on unsafe fallback. B_KEEP_LOADED alone does not retain vector/MSI/PCI/mappings. Mask after drain with proven hardware ordering, address storms and late delivery, audit no-sleep/lock ordering and retain all dependencies on failure. Design review only; see `/home/dan/renkuos-terra-vulkan/IRQ_LIFETIME_BOUNDARY.md`.

### Terra: proposed dispatcher-tail lock extension

Extending vector_lock through update_int_load appears to protect accounting of dispatchers that have already acquired the lock from concurrent free clearing assigned_cpu, provided locked mode is stable. No reverse load_lock/vector_lock order found in interrupts.cpp; evaluate increased IRQ-disabled hold time. Pending entrants, late IRQs, DEBUG empty-list path, mutable no-lock state and MSI/hardware quiescence remain separate. Design only, tests required; details `/home/dan/renkuos-terra-vulkan/IRQ_LIFETIME_BOUNDARY.md`.

OFFLINE PRIVATE GPU PAGE-TABLE ENCODER — 2026-11-07
Implemented isolated IntelN100Ppgtt.h: checked4KiB system-memory leaf/directory
encoding, raw PAT selector,39-bit DMA limit,48-bit GPU ranges and four table
indices. Outputs unchanged on rejection. No production/display integration.
Terra independently researched pinned i915 and accepted design before coding;
review caught unfixed enum invalid-value conversion, corrected to uint8_t enum.
Terra independently reran corrected2150cases with UBSan and accepted offline
milestone. Source references/design in research/n100-submission/PPGTT-ENCODING.txt.
Compiled: host C++17 warnings-as-errors+UBSan2150pass; nativeN100 C++17 warnings-
as-errors2150pass, build/test exit0, exact source/header hashes matched. Build
and execution visible viaNanoKVM. Physical machine ran CPU-only offline tests.
Simulation: no GPU simulator; tests check pure encoding/arithmetic. No installed
GPU mappings, real DMA, GPU workload or hardware-acceleration proof. No new VM test.
Remaining before hardware publication: owned pinned page backing/DMA translation,
scratch, actual PAT/cache setup, barriers/TLB invalidation, permissions, context,
completion/retirement/reset, and outstanding lifetime safety gates. General memory
close/unload and independent prior physical-evidence review remain incomplete.
Next: owned private page-table construction using reviewed encoding, still offline
until resource lifetime and hardware publication contract are validated.

OWNED OFFLINE PPGTT CONSTRUCTION — 2026-11-07
Implemented IntelN100PpgttTree.h: fixed ownership ledger for4active table pages,
3scratch table pages,1private scratch data page and1..16owned diagnostic pages.
One2MiB-aligned raw48-bit GPU window. Full scratch-backed tree; all CPU/DMA/token
validation precedes page writes. Failure rolls back known allocations; failed
release retains ledger/context for explicit retry. Ambiguous descriptors retain
all claims with no guessed cleanup. No destructor cleanup or publication API.
Backend and record lifetime, external serialization and callback non-reentry
are explicit requirements; backend must guarantee exclusive physical backing.
PAT selectors restricted0..3 per pinned N100 platform policy. Scratch aliases
are writable present mappings, not faulting guards or verified isolation.
Compiled/tested: host C++17 warnings-as-errors+UBSan69cases passed; independent
Terra run also69passed. Actual N100 native build/test exit0, matching input hashes,
visible NanoKVM. Full independent entry/walk checks; all24acquisition failures,
all24release failures/retries,rollback failure,8ambiguity variants and admission.
Simulation: allocator/DMA descriptors are modeled. Native execution allocates
ordinary userspace test RAM; it does not install real GPU tables or access GPU.
No new GPU ioctl, VM run, physical execution or acceleration result. Existing
production driver/display remains unchanged; this is an isolated candidate.
Terra accepted design before implementation and final corrected code after
independent tests. Coverage additions included uncached directories,1data page,
distinct PATs,invalid cache and48-bit boundary; no blocking review defect remains
within offline scope. Hardware readiness is not approved.
Next: real retained kernel-page allocator and DMA translation adapter, then
cache/PAT/TLB publication contract. Context, completion/reset and existing
lifetime gates remain required before any GPU workload.
Evidence: tests/evidence/ppgtt-tree-001/

BRING-UP ADDRESS/POWER AUDIT — 2026-11-07
Implemented documentation of exact GT forcewake handshake/domain and a source-
verified DMA-address contract. Terra independently reviewed both; stricter MMIO
error handling and2.5second aggregate deadline are proposed adapter policy, not
Linux equivalence or a validated physical response limit. The x86 PCI ram_address
identity helper is for resource conversion, not a per-device DMA mapping API.
Archived log has DMAR presence; applicable units/scopes and platform constraints
remain unverified. Inactive remapping alone would not establish full DMA readiness.
Compiled: none new. Simulation: none new. Physical: SSH recheck timed out; cached-
looking NanoKVM terminal unchanged, no input/reset capability recovered. No new
kernel fixture installation/run, MMIO writes, GPU workload or acceleration.
Next: physical reset enables the reviewed wired-page fixture; then live DMA and
GT ownership/power/forcewake checks before engine initialization/submission.

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

Terra GT power finding accepted for next design: retainedowner is coordination
point,but hasnoGTwakeref. ExistingPCI D0setter doesnotpropagateunderlyingwrite
failure orverifytransition;missingPMcap readsD0bydefault. These operationscannot
standinforLinuxruntimePM references. Next auditpower/suspendownership beforeforcewake
writes;read-onlyPCIpreflight doesnot requirebusmasterenable.

A26 findings accepted by Astra
Owner-local observation sessions coordinate participating clients only. Kernel
source audit found no ordinary supported suspend/hibernate path or transition
lease service; raw ACPI/PCI users and firmware remain outside control. Checked
PCI PM caps avoid GetPowerstate's absent-cap D0 default and unverified setter.
Actual hardware stable sampled config:8086:46d1 rev00/VGA, command0x407, BAR bf000004,
PM atd0/D0. This does not establish GT readiness or persistent DMA identity.
Pinned Linux intel_step.c adlp_n_revids[0]=A0; GT runtime power/RC6 services have
no RenkuOS equivalent yet. Next narrow observation candidate is raw ACK130044
(owner bounds/type/order checked); do not speculatively read protected request
registera188 or copy Linux's broad early0xefff reset into active-display boot.
ACK samples prove neither request-bit ownership nor a working handshake.
Pinned sources: https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/intel_step.c
https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_gt_pm.c

A27 Terra cache/read findings accepted by Astra: default physical mappings
request weak UC and may adopt an overlapping range's type; area_info hasno
type getter. N100 Intel GART maps same BAR strongly UC before owner. Clones
inherit source type. A27 makes UC explicit on existing owner map at fresh
boot, no extra alias/live mutation. x86-64 read barrier is LFENCE; this is
not a proven write/posting protocol. Physical raw ACK0 eight times doesnot
prove request bit0/fallback15 free or exclusive, no-firmware/SMM actors,
GT wake/readiness or DMA. Next handshake needs boot-scoped participating
bit authority, whole-sequence pin, closed admission on uncertainty, bounded
masked-bit cleanup for definitely-issued requests and retention for unknown
writes. No broad reset mask; fallback can remain separate until reviewed.

A28: same-device posting ACK is required after each issued request store before
cancellation/cleanup decisions; CPU store alone doesnotprove acceptance. Stage
deadlines must cover mandatory readbacks,including immediate desired ACK.
Late zero after unacknowledged enable cannot establish release;keepUnknown.
Final owner PCI/display validation can downgradeReleased toUnknown and must
retain pin with explicit validation-stage report. IO callbacks cannot persist
stack context beyond synchronousRun. Targeted source audit's apparent palette
alias resolves to4a188 via capturedPCH40000;no firmware/SMM exclusion inferred.
Single physical primary handshake demonstrated ACK response/release only.
