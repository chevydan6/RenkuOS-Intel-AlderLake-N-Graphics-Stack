<!-- SPDX-License-Identifier: MIT -->
# First verified N100 GPU execution: implementation plan for Astra

2026-11-07. Proposed design, independently researched by Terra. Astra owns
implementation and hardware deployment. No implementation or hardware pass is
asserted here. [Architecture](INTEL_N100_GPU_ARCHITECTURE.md),
[packet evidence](GPU_COMMAND_SUBMISSION_RESEARCH.md),
[blocking memory review](GPU_DRIVER_REVIEW.md).

## Scope and design delta

First prove a nonce-dependent MI store by the BCS0 command streamer, with ordered
completion and safe retirement. Use a kernel-constructed direct ring payload.
This is smaller than the separate batch proposed in Astra's existing
`research/n100-submission/DESIGN.md`; the Linux LRC selftest demonstrates this
form. Astra should accept or reject this simplification in coordination/DECISIONS.md.
No 3D pipeline, shaders, BLT format, userspace command parser or Mesa is needed.
A valid logical context/address-space root and managed DMA lifetime remain required.

## Ordered implementation checkpoints

Progress update, A34: one physical reset-only trial passed with two reset rounds
and clean four-domain release; postchecks passed. Evidence:
`tests/evidence/physical-bcs-reset-002/`. G2 still requires reinitialization and
context-status/error service. A33's earlier safety failure remains unexplained.
Direct-ring simplification is accepted in `coordination/DECISIONS.md`.

| ID / owner | Implement | Acceptance evidence |
| --- | --- | --- |
| M1 / Astra | Correct nested GART lock, hash insertion failure handling and semaphore cleanup; review partial-bind and failed-unmap callers | Actual helper/header in host tests; allocation fault matrix; native/kernel tests; new memory path physically validated separately |
| M2 / Astra | Kernel-owned buffer/request records, fixed trusted operation, close/unload retention | No request frees pages while DMA may access them; test every acquisition/unwind and client exit |
| G1 / Astra | Exact device/stepping gate, read-only GT/BCS inventory, forcewake and power acquisition with bounded ACK waits | Logged identity and capabilities; injection tests; no inherited legacy ring submission |
| G2 / Astra | Engine stop/reset/reinitialize, errors and context-status service | Successful bounded reset-only physical trial before payload; failure leaves service wedged and allocations quarantined |
| V1 / Astra | Owned scratch/root/tables, GGTT maps for context/ring/status/result, explicit PAT/MOCS and TLB invalidation | Address-domain ledger, page/table dump, bounds and alias tests; reviewed visibility ordering |
| C1 / Astra | Gen12 XCS LRC image, full context backing, ring head/tail/control, Gen11+ descriptor tags and CSB state | Byte/field review against cited code and target PRM; no stale tags across reset |
| S1 / Astra | Publish ELSQ descriptor(s) after memory barrier; clear unused ports; issue load once | Logged prepublication bytes/hash and descriptor; no publish on any prior failure |
| Q1 / Astra | Nonce and complement stores; Gen12 flush/breadcrumb; bounded completion/CSB polling and retirement | Disabled-submit negative control fails; real submission yields exact data, matching status, context inactive |
| H1 / Astra executes, Terra reviews | Isolated physical trials under canonical resource locks | Fresh physical identity, binaries/source hashes, register/status evidence and reproducible result logs |

Terra reviews each major design before implementation and checks implementation
and evidence afterward. These are responsibilities and gates, not assertions
that any code exists. Shared task ownership is recorded in coordination/TASK_BOARD.md.

## Required memory and address ledger

For every allocation record requested size, allocated size, owning object,
CPU VA/cache type, DMA pages/address limit, GGTT interval (if any), PPGTT interval
(if any), pin count, publication generation and release condition.
Keep result, status and context/ring separate from framebuffer/stolen mappings.
Do not expose kernel pointer, GGTT write target or command bytes to the client.

For direct-ring proof, reserve a dedicated 4-KiB result page and separate status
storage with guard patterns. Linux uses an 8-KiB copy context image plus two 4-KiB Gen12 workaround
pages (16 KiB before debug redzone); see the submission research addendum.
Cross-check context allocation and ring minimum against target documentation.
Build a valid four-level PPGTT/scratch hierarchy even if no payload uses it.
Use only 4-KiB mappings initially; huge-page and 64-KiB support can wait.

Allocate all records before publishing DMA-visible work. On prepublication error,
unwind in reverse order. On timeout, stop submissions and retain resources until
engine/context quiescence is established. Failed reset means quarantine, not free.
Closing the diagnostic client must not reclaim device-owned in-flight allocations.
Module unload must wait or be refused through a supported kernel lifecycle;
module references cannot be assumed to pin arbitrary caller-owned pages.

## Candidate request sequence

1. Take device submission ownership; reject cleanup-pending/wedged state. Acquire
   required power references. Select a fresh request sequence and reset generation.
2. Poison result/status and guards; generate fresh nonzero nonce differing from
   poison. Emit two MI stores into the kernel ring (nonce and complement).
3. Emit reviewed Gen12 XCS flush and completion sequence. Initialize any scratch
   semaphore read by the chosen tail sequence; do not copy Linux semaphore waits
   without implementing their CPU-side ownership and clearing protocol.
4. Finish context/ring image and page tables; apply CPU publication/cache barriers,
   required translation invalidation and posting reads. Log immutable ring bytes,
   mapping ledger and descriptor. The last fallible software step precedes publish.
5. Write ELSQ slots with valid descriptor in the selected port and zero unused
   slots, then issue load. Do not submit through inherited legacy tail registers.
6. Poll expected sequence and context status using reviewed GPU-to-CPU visibility.
   Proposed experiment deadline: 250 ms request, separate 1 s recovery budget;
   these are engineering limits, not hardware timing guarantees. Bound every MMIO poll.
7. Compare payload and all guards; verify exact request/reset generation. Check
   matching CSB transition and engine inactivity before retirement/free. Record
   completion-with-lost-IRQ separately from actual GPU noncompletion.
8. Release request resources only after retirement; preserve error evidence before
   reset. Reinitialize context/CSB/tags after reset, and reject late old completions.

## Physical validation protocol and evidence

Astra must verify recovery access and a known-working boot option immediately
before use; historical NanoKVM or SSH status is insufficient. Use isolated test
media and matched driver/backend artifacts. Preserve build 057 display behavior.
Hold the existing canonical hardware/VM locks for the entire operation/cleanup.
Terra performs no independent deployment.

Every trial log must contain:

- Evidence class: PHYSICAL, exact PCI ID/revision and machine identity; boot/build
  identity, source and binary hashes, firmware loaded/not loaded and submission mode.
- Result CPU/DMA/GGTT addresses, context/ring/VM address ledger, request/reset IDs,
  ring bytes/hash, descriptor(s), command publication time and bounded poll results.
- Poison, nonce/complement, expected/actual result and guards; completion sequence,
  CSB write/read positions/events, context ID, engine/fault state and retirement.
- CPU result write audit: initialization only writes poison; verifier only reads;
  no fallback path fills expected results. Record error/reset outcome and retained
  allocations. Include display and management-access checks after the trial.

Run the disabled-submission negative control through the same verifier: unchanged
poison must fail. Then perform one submitted trial, inspect evidence, and expand
to 100 fresh nonce trials with varied owned pages. A matching nonce alone is
insufficient: require completion and retirement evidence. An independently
reviewed harness must reproduce pass/fail from captured data. These controls
exclude ordinary CPU fallback; software logs alone are not cryptographic proof
against deliberately falsified instrumentation.

## Validation specifications

All cases below are **specifications**, except the R1 host reproducer and corrected-helper threaded validation
explicitly reported in GPU_DRIVER_REVIEW.md. Mocked services never establish
hardware execution, cache coherence, forcewake correctness or reset safety.

| Test | Host/simulation oracle | Native/kernel/physical counterpart |
| --- | --- | --- |
| Concurrent query vs unmap | Actual production lookup plus real nonrecursive lock; barrier inside lookup/use, teardown blocks; no UAF; invalid ID completes boundedly | Kernel worker stress on isolated boot, watchdog and semantic result checks |
| Concurrent allocate/bind/free/reopen | Deterministic interleavings; manager then aperture lock; no use after unmap, duplicate ownership or cleanup-only allocation | Multi-thread kernel callers, no CPU aperture writes racing live GPU DMA |
| Failed allocations | Fail record, hash initial bucket, page array, page run, context, ring, PT and status allocation in turn; outputs unchanged; no publish | Native allocator fault injection with baseline resource counts |
| Partial bind / rollback | Fail bind at every page, then unbind at every prefix position; retain accurate bound prefix/backing and recovery ownership | Backend fault injection; physical mapping validation separately |
| Cleanup / client death | Close before publish, after publish, after payload, during reset and quarantine; exact-once release only after quiescence | Native team death and controlled unload/reload; verify live references |
| Failed initialization | Fail each stage, forcewake ACK and register check; unwind completed stages, balance modules/semaphores; retry cleanly | Reset-only physical trial and matching display/management baseline |
| Timeout / lost IRQ | Suppress notification while status completes versus freeze status; only genuine noncompletion enters reset | Mask notification in controlled test; bounded completion polling; no intentional initial hardware hang |
| Reset failure / stale events | Inject failed reset, old CSB/seqno after new generation; quarantine survives close; new submissions rejected | Controlled physical recovery only after reset design reviewed; retain logs/pages |
| Post-error recovery | After successful simulated reset, fresh context/tag runs request; repeated cleanup failure stays tracked | Subsequent fresh physical nonce trials after proven quiescence and reinitialization |
| First execution control | Disabled publish fails verifier; mocked successful store labeled SIMULATION | Physical result + ordered completion + matching context event + guards; unchanged baseline desktop |

## Remaining prepublication research gates

Exact forcewake/power/reset register sequence, target stepping workarounds,
context sizes, full CSB initialization, PAT programming and coherent CPU mapping
contract remain open. The Linux MOCS source was retrieved and inspected here;
that resolves source availability, not target hardware validation. Astra must
supply a field-by-field initialization design before Terra can independently
approve a hardware-ready sequence. First physical execution remains pending.
