# Cycle 74 — native builds verified; VM verification repair pending

Chain ID: `cycle-e41db642-fff3-4fb8-abe6-d7efbf5d6d5e`. Integrator outcome: **repair**.
Milestone `native-driver-baseline`: **incomplete**.

Local source preparation exited 0. Both baseline native builds exited 0 (7 driver and 23 accelerant compile units); independent inspection verified the retained artifacts. Source checks compared 1,206 published files and 967 prepared payloads, including 118 ordered patches. Source audit: 8 PASS, exit 0. ELF64 x86-64 shared-object headers, dependencies, required exports and SHA-256 checks passed.

Host fixtures passed **347 checks, zero failures**: 280 kernel-resource and 67 GART-allocation checks using extracted methods with mocked allocations on Linux.

Native VM verification failed with exit 1 before SSH: the verifier lacked the existing writable VM folder attachment and could not open the canonical lock (EROFS). No guest command ran; guest payload/hash checks, `hooks-test` and `kernel-resources-test` remain pending. This is a worker attachment issue, not evidence of VM or host disk failure.

Live reachability was not measured this cycle. No image deployment or boot verification occurred. No physical hardware operation or verified GPU execution occurred. Host fixtures and ELF checks do not establish Haiku loadability or GPU execution. The native build used the installed VM SDK/private headers and kernel library; the pinned source subset was checked, without rebuilding the full OS/SDK.

Source commit: `30dfdb32957215b4c59da3ac763ba14ab0c482d0`. RenkuOS dependency commit: `4799bb5a188c4bb6bbef967287640cc21a061a0d`.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `intel_extreme.driver` | 255952 | `3b0f017fefa6979ba3c70cd785848fbf88c36e4f818cd12272c4927748123080` |
| `intel_extreme.accelerant` | 1148768 | `31a8ab0446124c1430afc719f0f37ec98e35071f03b1b62e1c3b7d7ffe91becc` |

Next action: Relaunch the configured verifier with the existing writable RenkuOS VM folder attachment, then execute the saved argv in evidence/native-cycle-74/independent-vm-command.json through canonical vm.py run (85-second limit) to check guest payload, binary hashes and run hooks-test plus kernel-resources-test; retain output/status and finalize native-driver-baseline.

Selected workspace-relative evidence references (retained locally):

- `evidence/native-cycle-74/publisher-handoff.txt`
- `evidence/native-cycle-74/independent-verification.json`
- `evidence/native-cycle-74/source-verification.json`
- `evidence/native-cycle-74/driver-build.log`
- `evidence/native-cycle-74/accelerant-build.log`
- `evidence/native-cycle-74/binary-inspection.log`
- `evidence/native-cycle-74/independent-host-tests.log`
- `evidence/native-cycle-74/independent-vm.log`
- `evidence/native-cycle-74/independent-vm-command.json`
