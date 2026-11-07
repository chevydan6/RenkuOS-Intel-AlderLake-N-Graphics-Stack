# A34 physical BCS reset success — 2026-11-07

Evidence class: PHYSICAL, N100 8086:46d1 revision00, `shredder`, RenkuOS
hrev60072+518, USB root `/dev/disk/usb/0/0/1`. Source patches0001–0096.
One guarded reset-only invocation; no engine initialization or command submission.

## Artifact identity and preparation

* Owner: `5a7dee1a965cca667ddb8ff46a704b18ca1f87559fe942c663d285ac60c127c6`
* Bridge: `2d37e02f4e87c87ad046b257b00a43dd7f7e2bcc95853d765b20d0234475eb21`
* Client: `ccefa7b73023f2d5c95ecc98ac7e76e4840501db5f02701ff66e26a66258359a`
* Kernel: `66753cb36a1481790dbf55a564e8632fdcb7534fdacbb6f5999432ea196de4f6`

96-patch reconstruction matched tested source;843 payload hashes passed. Native
VM140/431/964/4101/84/37 and host140 actual-owner sanitizer cases passed; see
`../reset-safety-host-vm-001/`. Eight A32 rollback files were hash-verified on
workstation and target. Installation, reboot and invocation ran visibly through
NanoKVM under the canonical physical lease. Fresh loaded owner path, uptime,
USB root and installed hashes were checked. Initial SSH timeout resolved after
outgoing console pings; KVM showed a responsive desktop.

## Observations

`physical/sample.log` and `sample.exit`: exit0, protocol2, transaction0.

* Fuse `0x000e00fe`; bootstrap ACK0→1→0, two writes,479us.
* Four-domain group Complete,2383us; GT/render/VDBOX0/VEBOX0 all Released,
  two writes each, final primary/fallback ACKs clear.
* ResetObserved:15 reads,6 writes,2 reset rounds,1150us. Final own reset
  request cancellation `0x220d0=0x10000`, final read `0x220d0=0`.
* Retained safety snapshot: reasonNone,49 checks,196 ACK reads; stable PCI
  identity/command/BAR/D0. Snapshot ACK ones were sampled during Work;
  the group's final release observations are zero.

`physical/postchecks.log` and `postchecks.exit`: exit0. Clear GT ACK, PCI/D0
lease tests, BCS inventory (tail/head/control0, mode0x200, message-idle0x10)
and ten paired-GART rounds passed. Candidate, kernel, display, GART and previous
bridge hashes verified in `postcheck-system.log`. Final NanoKVM observation
showed responsive windowed Terminal, desktop and Deskbar with postcheck exit0.
Lease PID314 was ownership-checked and explicitly released; see `lease.json`.
A34 remains installed; A32 rollback artifacts remain preserved.

Archive: `build/a34-trial-001/physical-results.tar.gz`, SHA-256
`5169dcaf1e4cd357fdb1a559f37875b2685c82fc60494b4dcfc722f19c479f93`.

## Limits and next prerequisite

A33's generic Unsafe condition did not recur. Its cause is unresolved; added
diagnostics and this success do not establish the cause or workload reliability.
Reset has not initialized a runnable engine/context. Next: field-by-field
context/status and engine-reinitialization design, owned memory construction,
cache/TLB policy and retirement service before any descriptor publication.
First GPU command execution, acceleration and Vulkan remain pending.
