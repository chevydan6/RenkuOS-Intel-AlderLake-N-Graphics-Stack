## Astra High


### Astra publication checkpoint — 2026-11-07

Publication assignment completed: recorded GPU milestones are on the existing
private GitHub project; local commit history and production worktree preserved.
Implementation: archive missing reviewed recovery patches/harnesses and current
coordination records; add a concise current report and refresh README status.
Compilation/testing: full patch reconstruction and existing host CI checks pass;
no new native compilation or physical test was performed for publication.
Physical status: A28 remains one successful primary forcewake handshake; A29 is
host-only and not installed. No GPU command execution has been demonstrated.
Blocker for A29: target SSH/transfer and NanoKVM HID failed; cause unknown,
physical reset requested. Publication can proceed independently of target access.
Publication: both gpu/native-memory and default gpu/renkuos-n100 were verified
at 767fd36; GitHub source workflow 42347031752 passed all steps (29 seconds).
Tracking issue: https://github.com/chevydan6/haiku-intel-gpu/issues/1
Next: resume A29 native validation when laptop access is restored.

Current: A29 offline candidate complete/reviewed; native validation blocked on laptop access.
Source: gpu-next-worktree; implementation Astra, independent review Terra.
Implemented: held-GT fixed5read BCS inventory, typed uncertainty and final-retirement-only output;
v3 unchanged. Commitsfa6102a/96fd967 are explicitly host-only candidates.
Compiled/tested:84 state+37sampler+48actualowner host ASan/UBSan cases pass independently.
Native: NOT COMPILED. Source transfer timed out before build; no A29 deployment.
Physical: A28 single forcewake handshake andpostrelease checks passed historically.
Current SSH/HID stalled; SSHbriefly recovered at24minuptime, software-reboot attempt
failed to restoreaccess. NanoKVM HIDg0/g1 timeout; onlyvideo/HIDcontrols exposed,noATX.
Dan physicalreset requested, RenkuTestUSB attached. Cause unknown;no sleep/crash cause claimed.
Next after recovery: native source/fixture build, artifactreview/backup/freshboot andfixedrawtrial.
No A29 physicalengine samples,GPUcommands,DMA oracceleration. Engine/context/reset gates remain.

## Terra Low
Owner: Vulkan userspace research/integration and independent review; Astra owns kernel.
Worktree: /home/dan/renkuos-terra-vulkan (independent repository, research/vulkan-userspace).
Current: software Vulkan loader, offscreen raster and compute validated in disposable native VMs;
HaikuPorts Lavapipe CPU chosen by loader; negative controls pass. No physical GPU work.
Completed: VULKAN_BASELINE.md, INTEL_ANV_PORTING_ANALYSIS.md, draft GPU_VULKAN_INTERFACE.md,
VULKAN_PROGRESS.md and VULKAN_WSI_RESEARCH.md. Proposed kernel requirements await Astra review.
Blockers: no GPU command execution, no agreed ANV kernel ABI, DMA/cache/TLB/reset contract,
no verified native surface extension, physical target/recovery access.
Next: third-party headless application compatibility and deeper ANV interface analysis;
independently review Astra kernel work when requested. VM use requires canonical locks.

### Terra independent review: A26
Reviewed the retained-owner v1 source, PCI capability and PMCSR checks, and
actual-function concurrency harness. Independently reran 302 helper cases and
10 mocked owner cases with ASan/UBSan. The malformed PM capability overlap was
fixed and retested before native integration.

Accepted physical evidence for six archived PCI sessions (IDs 2–5 and 8–9)
and ten paired GART rounds with 20 allocations/frees. Reviewed source and
installed binary hashes match; the old owner backup matches the prior baseline.
The intermediate IDs 6–7 are excluded because their log was overwritten.
Post-boot through final captures retain app_server PID 127 and record unchanged
display/GART component hashes. These results do not establish a system power
hold, GT access, GPU DMA identity, or GPU execution.

Next review: an owner-only, bounds-checked GT ACK inventory at BAR0+0x130044,
with mapping/order and controlled-boot constraints reviewed before physical
access. ACK samples cannot prove request-bit ownership or a working handshake;
forcewake writes remain gated on power, shared ownership, stepping, and recovery.

### Terra independent review: A27
Reviewed the retained-owner v2 fixed-offset ACK observation, explicit uncached
BAR mapping, bounds and lifecycle, and independently reran 26 actual-function
mocked owner cases under ASan/UBSan. Native source/header/fixture/generated-test
hashes match the reviewed inputs; native build and 26 mocked cases passed.

Accepted physical evidence: eight raw reads at BAR0+0x130044 returned zero
across the live and post-memory runs; each client exited successfully. Ten paired
GART rounds with 20 allocations/frees passed, PCI observation remained valid,
and the post-boot app_server process and display/GART component hashes were
preserved. The matched prior owner backup is retained. These are ACK samples,
not proof of request-bit ownership, a power hold, or GPU execution.

A28 next: independently review the forcewake request/fallback ownership and
power assumptions, bounded handshake, and distinct cleanup for definite versus
uncertain writes. No forcewake stores or hardware handshake are yet established.

### Terra independent review: A28
Reviewed the primary-only forcewake state machine, retained owner adapter and
single-transaction fixture. Mandatory post-store ACK ordering and 50 ms stage
deadlines were corrected and independently retested. Host ASan/UBSan and native
mock runs passed 67 state-machine and 37 actual-owner cases. Implementation
commits: 9bb30b3 and cc5befd.

Accepted evidence for ONE controlled physical primary-bit handshake: the owner
reported ACK 0 -> 1 -> 0, nine reads, two ledgered writes, 454 microseconds,
Released and exit 0. Reviewed source and installed binary hashes matched; the
prior owner backup was preserved. Four subsequent read-only ACK samples were
zero, PCI checks passed, and ten paired GART rounds freed 20 allocations. The
post-boot through final captures retained app_server PID 127 and unchanged
recorded display/GART component hashes. No independent bus trace was captured.

This result does not establish fallback, repeated reliability, global power or
firmware ownership, reset/quiescence, DMA safety, or GPU command execution. A29
is the next held-access engine inventory/design review; no further hardware
writes were performed in this review.

### Terra independent review: A29
Reviewed the fixed five-register BCS sampler, typed uncertainty handling,
retained-owner v4 lifecycle, and the single-transaction native fixture. Host
ASan/UBSan runs passed 84 forcewake, 37 sampler, and 48 actual-owner mocked
cases. Uncertain sampler results retain the operation pin; successful raw
samples are published only after release and final owner validation. The fixture
does not enforce a system-wide one-shot limit. Source commits fa6102a and
96fd967 are host-only candidates.

Native compilation has not run: source transfer timed out before build, and
no v4 owner was deployed or BCS register sample taken. SSH briefly recovered
with 24 minutes of uptime; the later software reboot attempt did not restore
SSH or NanoKVM HID access. The cause is unknown, and a physical reset has been
requested. A28's earlier physical handshake remains historical evidence only.

Investigated pinned i915 BCS stop/reset and Gen12 workaround prerequisites as
research only. Engine and power ownership, other command-streamer coordination,
stepping-specific requirements, and recovery remain unresolved; no reset or
GPU workload is authorized by these findings.
