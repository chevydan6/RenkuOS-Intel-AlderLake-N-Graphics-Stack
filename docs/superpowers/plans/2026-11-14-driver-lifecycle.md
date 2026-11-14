# Native BCS driver lifecycle implementation plan

> Execute inline with the executing-plans skill; preserve the user-requested branch.

**Goal:** Handle stalled requests, device errors, client exits, concurrent
clients, shutdown and suspend/resume; add interrupt completion if measurements
show benefit on the working path.

**Architecture:** Keep one resident, bounded GPU backing pool and one request in
flight. Separate admission state from hardware retirement. Recover only after
Intel's stop/reset sequence proves quiescence; otherwise retain backing and
require reboot. CPU staging belongs to each open file and never becomes DMA
backing. Preserve the observed VT-d-disabled admission restriction.

**Spec:** User's driver lifecycle request and `NATIVE_GPU_HANDOFF.md`.
**Tech stack:** Native RenkuOS C++ kernel module/legacy bridge, Linux v6.6 Intel
hardware references, portable focused C++ fixtures and physical N100 validation.

## Constraints and review focus

- Preserve Intel hardware sequences and source attribution.
- No unrelated hardening, exhaustive tests or shader work.
- A reset attempt alone never grants permission to reuse memory.
- Never report successful copy data after a timeout or failed verification.
- Concurrent admission, client close during work and power drain must agree on
  the same active request and backing lifetime.
- Preserve the failure report; do not report an earlier request's LOAD as new.
- Driver suspend/resume boundaries do not prove actual system suspend/resume.

## Tasks

### 1. Explicit admission and lifetime

Files: `experimental/native-gpu/RequestLifecycle.h`, `lifecycle_smoke.cpp`,
`NativeExecution.inc`, `ExecutionModule.h`, `execution_bridge.cpp`.

- [x] Write and run a failing fixture for single admission, successful reuse,
  uncertain failure, admission freeze/drain and terminal shutdown.
- [x] Implement a lock-external state policy with counters and generations.
- [x] Integrate a request scope so every accepted request reaches a terminal
  state, including allocation/bootstrap failures; expose fixed-width status.
- [x] Give each bridge open a resident-module reference; close rejects later
  operations and waits for the synchronous call before staging is freed.
- [x] Verify targeted portable fixtures and native compile/link.

### 2. Bounded recovery

Files: new `BcsRecovery.h`/`recovery_smoke.cpp`, `NativeExecution.inc`.

- [x] Pin Intel stop-CS/reset-prepare/double-domain-reset/cancel behavior with a
  failing simulated register fixture, including a failed-reset path.
- [x] Reserve time inside the held forcewake group for recovery after timeout
  or device error. Preserve the original failure and backing ledger.
- [x] Reopen admission only after quiescence, power release and owner/DMA checks;
  otherwise enter Lost. Reinitialize engine/context after successful reset.
- [x] Exercise an explicitly identified completion-observation timeout on native
  hardware and verify a later independent copy; do not claim a true hang test.

### 3. Client concurrency and power lifecycle

Files: lifecycle API/header, owner, bridge and narrow native lifecycle client.

- [x] Serialize/bound competing submissions; distinguish busy from device lost.
- [x] Verify actual VFS close/free guarantees, then native concurrent clients
  and client termination without freeing GPU-visible memory.
- [x] Root-controlled drain/shutdown/suspend/resume boundaries stop admission
  before draining; resume revalidates owner, power, DMA and engine state.
- [x] Reject admission once the exported OS shutdown flag is set. Provide tested
  driver quiesce/resume boundaries. Actual system suspend/resume is excluded:
  user explicitly selected driver work first and **no kernel work**. The pinned
  OS does not dispatch driver power callbacks and ACPI wake currently panics.

### 4. Completion measurements and evidence

- [x] Measure polling wait, sleep and cache-maintenance costs on the real path.
  Existing polling sleeps 100 us; implement IRQ only if useful and coordinate
  ownership/acknowledgment with the display driver's shared interrupt handler.
- [x] Focused native copies, competing clients, timeout recovery, power boundaries
  and postchecks under the physical lease; preserve HDMI and boot media.
- [x] Review the completed change, record exact evidence and remaining OS gaps
  in `NATIVE_GPU_HANDOFF.md`; never claim actual S3 from manual boundaries.

## Execution record

- Initial inspection: branch `codex/i915-native-adaptation`, HEAD `0cdaeab`, clean.
- Linux v6.6 active reset differs from the strict cold-reset helper: do not
  reuse its disabled-ring preconditions for a stalled live engine.
- RenkuOS exports `gKernelShutdown`, `_get_team_info`, `acquire_sem_etc`.
- No new native code has been installed for this task.
- User scope clarification: driver work and tested quiesce/resume boundaries;
  no RenkuOS kernel/ACPI modifications.
- First native candidate compiles/links with `-Werror`; five existing focused
  fixtures plus lifecycle fixture pass. Not installed at that checkpoint.

- Final native candidate003: owner/bridge v3 installed together, booted and
  tested; native `-Werror` build/export link and seven focused fixtures passed.
- Native proof: first boot 75 admitted requests (73 successful, two controlled
  completion-observation timeouts), concurrent distinct-data clients, unchanged
  handles through quiesce/resume, kill while Running and drain/resume/reuse,
  terminal shutdown rejection; final boot four additional successful copies.
- Review fixed pre-admission staging ownership before installation; final
  independent review found no important safety/lifetime/reuse issue.
- IRQ decision: one CSB poll/no sleeps on warm path,120–149 us wait versus about
  1.4 ms cache maintenance and 0.6 ms submit; no demonstrated IRQ benefit.
- Completion audit: stalled requests are bounded and reset or Lost; persistent
  errors deny reuse; close/free and concurrent clients have native evidence;
  manual power boundaries have native evidence. Actual OS power dispatch/S3 was
  explicitly excluded by the user. No physical hang/fault or OS wake is claimed.
- Final source matches all 40 bundled C++/header/build files. Native archive
  SHA256 1dd9316a6ad0423153d555d57e3d77f36044a59561be7921f419f82d11712deb.
  Original 13 display/observer hashes match; app_server127, Ready sequence4;
  physical lease released after verified postchecks.
