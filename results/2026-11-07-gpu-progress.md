# Intel N100 GPU development progress — 2026-11-07

Historical A29 publication checkpoint. Superseded by the
[A34 reset / A35–A37 context progress report](2026-11-07-reset-context-progress.md).
The access blocker and pending A29 results below were subsequently resolved.

Target: Intel N100 / Alder Lake-N, PCI `8086:46d1`, revision 00. Production
implementation: Astra. Independent research, review and validation: Terra.

**One real GT forcewake handshake has succeeded. No GPU command submission,
hardware-accelerated rendering, or OpenGL support has been demonstrated.**
Display scanout and CPU-side kernel tests are distinct from GPU execution.

## Implemented and validated milestones

| Milestone | Implementation and evidence | Validation scope |
| --- | --- | --- |
| HDMI desktop | Native 1920×1080 RGB8 output; build 057 is the known-working display baseline. [Report](2026-11-06-hdmi-integration.md). | Physical laptop and NanoKVM video. |
| Memory and recovery | Retained aperture ownership after failed cleanup, serialized callbacks, rejected new work on retired apertures, checked allocation bounds and resource teardown. Builds 068/069 separated and tested the recovery/locking series. Later build 089 and physical paired-client/copyout tests extend the evidence. [Maintenance](../MAINTENANCE.md), [physical memory](../tests/evidence/physical-memory-002/README.txt), [copyout](../tests/evidence/physical-copyout-001/RESULT.txt). | Host fault injection, native compilation and selected physical allocation/cleanup tests. Full interrupt/unload lifetime safety is still incomplete. |
| A16–A19 private page tables | Checked 39-bit DMA / 48-bit GPU-VA encoding, owned offline trees and kernel wired-page provider. 2,150 encoding and 69 tree cases passed host/native tests. Three physical kernel cases exercised 42 wired areas. [Physical result](../tests/evidence/physical-kernel-pages-001/RESULT.txt). | CPU-side tests and real kernel memory allocation/walking/cleanup. No page-table publication to a GPU engine. |
| A20 first workload encoding | Fixed nonce/complement GGTT-store payload, 32 host checks. [Result](../tests/evidence/nonce-encoding-001/RESULT.txt). | Offline packet encoding only; never submitted. |
| A23–A25 DMA prerequisites | Physical DMAR snapshot, two read-only VT-d status samples and mandatory reserved-range exclusions in the page provider. All 42 physical page-exclusion checks passed with zero live areas afterward. [DMAR](../tests/evidence/physical-dmar-001/RESULT.txt), [VT-d](../tests/evidence/physical-vtd-001/RESULT.txt), [exclusions](../tests/evidence/physical-reserved-pages-001/RESULT.txt). | Physical observations and kernel allocation checks. Sampled TES-clear does not establish persistent DMA identity or cache/TLB correctness. |
| A26–A27 retained owner | Checked PCI power preflight, explicitly uncached owner mapping and fixed ACK observation. Six archived PCI sessions and eight ACK samples; paired GART regression tests. [PCI](../tests/evidence/physical-pci-owner-001/RESULT.txt), [ACK](../tests/evidence/physical-gt-ack-001/RESULT.txt). | Native builds, mocked owner tests and physical device reads. No general system power hold is established. |
| A28 forcewake | Primary-bit-only bounded handshake, atomic admission, mandatory posting reads and retained ownership on uncertainty. [Full evidence](../tests/evidence/physical-forcewake-001/RESULT.txt), commit `f4f2e60`. | Native owner/bridge/client compilation and linking; 67 state-machine + 37 mocked owner cases passed host/native tests. One physical handshake passed and Terra accepted the evidence. |
| A29 BCS inventory candidate | Five allowlisted reads under the held forcewake lifetime, typed uncertainty, bounded inspection and report publication after release/final validation. Commits `fa6102a` and `96fd967`. [Result](../tests/evidence/bcs-inventory-host-001/RESULT.txt). | **Host only:** 84 state-machine + 37 sampler + 48 actual-owner mocked cases passed ASan/UBSan and independent review. Native build, deployment and physical samples have not happened. |

## What the physical forcewake result proves

A28 used one client invocation, with no automatic retry. The recorded result was
ACK `0 → 1 → 0`, nine ACK reads, two software-ledgered writes, successful release
and exit 0. The state-machine interval was 454 microseconds; it excludes owner
admission and final validation/copyout and is not a performance benchmark.
The reviewed fixed request words were `0x10001` and `0x10000` at `0xa188`, with
ACK at `0x130044`. The exact stores are supported by reviewed source, installed
artifact hashes and the software ledger; there is no independent bus trace.

After release, four read-only ACK samples returned zero, PCI preflight passed,
and ten paired GART rounds completed 20 allocations/frees. The post-boot through
final captures retained app_server PID 127 and unchanged recorded display,
accelerant and GART binary hashes. The previous owner backup was preserved.
Native build and trial commands were visible through NanoKVM; SSH transferred
sources and collected logs.

This single trial does not prove repeated reliability, fallback forcewake,
firmware/SMM ownership, engine initialization/reset, safe GPU DMA, context
execution or accelerated rendering.

## Current implementation and compilation status

A29 samples BCS tail `0x22030`, head `0x22034`, control `0x2203c`, MI mode
`0x2209c`, and raw `MSG_IDLE_BCS` `0x800c`. It reuses the retained owner rather than
introducing a second register mapper. Its 250 ms total budget reserves time for
enable, inspection and cleanup. Uncertain results retain the operation pin and
numeric request until reboot. The candidate adds no engine/ring/reset stores.

The A28 owner is the last physically installed and validated owner. **A29 native
compilation has not run:** transfer timed out before the build, no candidate
was installed and no physical BCS sample was taken. There is no A29 VM result.
Host mocked tests remain simulations even when analogous tests run on the
physical laptop's CPU.

SSH briefly recovered at approximately 24 minutes of uptime. A later software
reboot attempt did not restore SSH or NanoKVM HID. The cause is unknown; neither
sleep nor a kernel crash is established. A physical reset was requested and is
the immediate access blocker. No NanoKVM ATX/power-control capability was
available in the verified tool inventory.

## Remaining prerequisites and first GPU experiment

1. Restore target access, preserve the known-working boot/recovery artifacts and
   finish A29 native compilation and controlled physical read-only inventory.
2. Establish stepping-specific engine/power ownership, required initialization,
   context state, and bounded reset/quiescence recovery using the pinned i915
   research. Linux interfaces are references, not RenkuOS facilities.
3. Close outstanding interrupt/dispatcher, module-unload and request lifetime
   defects before relying on asynchronous completion. Keep backing pinned until
   completion or proven reset; uncertainty must retain ownership.
4. Prove the chosen DMA address/translation contract, CPU/GPU cache visibility,
   GPU mapping/TLB synchronization, and completion/retirement behavior.
5. Only after those gates, submit the smallest reviewed nonce/complement store
   workload to owned scratch memory. Record pre-submit sentinels, the submitted
   packet/context/mapping identities, bounded completion, the independently
   checked result and cleanup/reset outcome. CPU fallback is not evidence.

The [design](../research/n100-submission/DESIGN.md),
[forcewake trial contract](../research/n100-submission/PRIMARY-FORCEWAKE-TRIAL.txt),
and [BCS inventory design](../research/n100-submission/BCS-INVENTORY-DESIGN.txt)
contain implementation details and historical decisions. Offscreen rendering
and Mesa/OpenGL integration follow verified execution; the working display path
should remain isolated from these experiments.

## Publication and evidence inventory

The GitHub checkpoint preserves the local milestone commit history, patches
through 0092, missing recovery patches 0069–0079, their host harnesses, the GART
caller audit and the coordination records. The complete work log is
[PROGRESS.md](../workstreams/gpu-next/PROGRESS.md). Shared records include
[agent status](../coordination/AGENT_STATUS.md),
[task board](../coordination/TASK_BOARD.md),
[technical findings](../coordination/TECHNICAL_FINDINGS.md),
[code review](../coordination/CODE_REVIEW.md),
[integration log](../coordination/INTEGRATION_LOG.md),
[hardware results](../coordination/HARDWARE_TEST_RESULTS.md) and
[decisions](../coordination/DECISIONS.md).
Earlier entries are historical; later evidence supersedes earlier pending states.

For this publication, all 92 patches reconstructed successfully from pinned
RenkuOS source `4799bb5a188c4bb6bbef967287640cc21a061a0d`. Host checks passed
payload verification, source-audit original controls, EDID selection, GGTT
range/PTE/stolen decoding, GART range/lifecycle/allocation, kernel-resource
suppression, boot-framebuffer bounds, offline copy-packet checks and Python
syntax. The lifecycle fixture passed 154 checks and its original-source control
reproduced 39 expected failures. These publication checks are host evidence,
not new native compilation or physical GPU tests.

Downloaded Linux reference source caches remain local; pinned source URLs and
hashes are in the research records. Credentials, private access configuration,
boot images, backup binaries and unreviewed private logs are excluded. Reviewed
evidence already selected for this repository includes logs and NanoKVM images.
No experimental code is merged into RenkuOS Source main by this checkpoint.
