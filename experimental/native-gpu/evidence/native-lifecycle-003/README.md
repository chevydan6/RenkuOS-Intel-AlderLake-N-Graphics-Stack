<!-- SPDX-License-Identifier: MIT -->
# Native BCS lifecycle and recovery — 2026-11-14

Base `0cdaeab`, branch `codex/i915-native-adaptation`.
Implementation and contract: [driver lifecycle](../../DRIVER_LIFECYCLE.md).
User scope: GPU driver/bridge work and tested quiesce/resume boundaries;
**no RenkuOS kernel/ACPI work**.

The physical N100 ran 75 admitted requests in one boot: 73 successful copies and
two deliberately hidden-completion failures. Both failures completed real BCS
reset/release and allowed later copies without reboot. After the terminal
shutdown test and reboot, four more copies passed. All successful owned-buffer
checks compared every destination byte against a client-specific changing source.

## Native checks

| Check | Authoritative result |
| --- | --- |
| Initial copy | [first-copy.log](first-copy.log): sequence1, stage14, all 1024 words, matching nonce/sequence, idle BCS, EIR/fault0, forcewake release successful. |
| Repeated copies | [repeated.log](repeated.log): eight distinct-data copies, sequence9, no failures. |
| Quiesce/resume | [quiesce-resume.log](quiesce-resume.log): quiesced copy rejected before admission; original handles remained CPU-accessible and passed four copies after reset/resume; generation2. |
| Concurrent opens | [concurrent.log](concurrent.log): two background clients, seeds53/87, 24 verified copies each; both exit0; sequence61, no failures. GPU execution stays serialized. |
| Completion timeout | [timeout-copy.log](timeout-copy.log), [timeout-status.log](timeout-status.log): sequence62 returns `B_TIMED_OUT`, CSB deadline, two BCS resets, Quiescent recovery stage11, normal forcewake release, Ready generation3; no successful-copy claim. |
| Reuse after timeout | [post-timeout.log](post-timeout.log): eight new-client copies, sequence70, without reboot. |
| Exit during active request | [client-exit.log](client-exit.log): status Running at sequence71 with two opens; SIGKILL exit137; quiesce drains timeout/recovery, reaches Suspended with only querying open remaining; resume generation5; four new-client copies reach75. |
| Terminal shutdown | [shutdown.log](shutdown.log): Shutdown state6; copy and resume rejected; sequence stays75, counters73/2. |
| Final boot | [final-boot-copy.log](final-boot-copy.log): four fresh copies pass; Ready, sequence4, generation1, no failures. |

The administrative timeout check hides CSB observation for one **valid** copy.
The actual GPU finished its data copy; the driver still returns failure, resets
the BCS engine and rebuilds submission state before reuse. This proves the bounded
timeout/reset/release/reuse path, not recovery from a physically hung engine.
No real device error, enabled-IOMMU trial or actual system suspend/wake was injected.

The portable recovery fixture checks Intel stop/prefetch/prepare ordering, double
domain reset, catastrophic-error handling, failed reset with cancellation, and
unsafe-owner refusal. The lifecycle fixture checks uncertain failure stays Lost,
verified reset permits a new generation, drain completion and terminal shutdown.
The buffer fixture verifies rejected admission preserves CPU ownership while an
admitted failed copy makes staging Unknown. Native `-Werror` compile/export-link
and all seven focused fixtures pass in [build.log](build.log).

## Why polling remains

Warm snapshots measured one CSB poll and zero sleeps. Wait cost was about 120–149 us;
whole-machine cache maintenance was1397–1432 us across three flushes, and descriptor
submission609–648 us. Warm accepted-request time was about 2651–2844 us. The timeout
case recorded20 ms waiting with144 sleeps and total23.2 ms including real recovery.

`total_us` covers an admitted request; bounded competing-client wait is reported
separately as `queue_us`. Timing includes validation/visibility/verification on
the measured path and is not intrinsic BCS execution time. Startup and module-open
cost are not included. These tiny-copy measurements do not establish larger-copy
throughput or an IRQ benefit; no interrupt ownership/masking changes were made.

## Exact artifacts and final state

- Native source/build/logs: `/boot/home/native-lifecycle-003`.
- [source.sha256](source.sha256): all 40 bundled C++/header/build sources match the
  current implementation; [installed.sha256](installed.sha256) and
  [installed-final.log](installed-final.log) identify verified resident candidates.
- Owner SHA256: `34b036a49eb692354fe94f42ced732ba4c957ef3433f272c8aadff3dd4e27315`.
- Bridge SHA256: `0e5963cad4781e12305ca492f278614f4fe88631298e50db1ebdb3a573786a8c`.
- [unchanged-final.log](unchanged-final.log): all 13 original display/observer
  component hashes match. [app-server.log](app-server.log): app_server PID127.
- [Final desktop](final-desktop.jpg), drained NanoKVM capture after final reboot;
  [kernel identity](final-uname.log). Native wall time is incorrect; do not use it
  to date this trial.
- Exact source/objects/binaries/raw logs archive:
  `/home/dan/projects/native-lifecycle-evidence-003.tar.gz`, SHA256
  `1dd9316a6ad0423153d555d57e3d77f36044a59561be7921f419f82d11712deb`.
  Matching native archive `/boot/home/native-lifecycle-evidence-003.tar.gz`.
- Previous owner/bridge saved in the native bundle and identified by
  [previous.sha256](previous.sha256). Both v3 components must be replaced together;
  resident-code replacement still requires reboot.
- Physical lease `native-lifecycle-001-20261114` was released after verified
  postchecks. HDMI build057, display driver, accelerant and boot media are preserved.

Read-only review before installation found one pre-admission staging-ownership
bug, fixed with a failing-then-passing fixture before building this candidate.
Final independent review found no important safety/lifetime/reuse issue. No source
in the RenkuOS kernel or display stack was modified. Persistent errors still leave
Lost with retained backing and reboot recovery, rather than resetting other engines.
