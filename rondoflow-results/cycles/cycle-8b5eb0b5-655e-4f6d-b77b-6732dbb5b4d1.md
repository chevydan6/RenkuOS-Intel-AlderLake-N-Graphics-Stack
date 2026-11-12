# Cycle 77 — repair required

Chain: `cycle-8b5eb0b5-655e-4f6d-b77b-6732dbb5b4d1`. Native builds and independent artifact checks passed. Patch 0122 adds partial ioctl serialization; production request lifetime wiring and actual-entry-point tests remain incomplete. The accepted native baseline is preserved. GPU execution and DMA safety remain unproved.

## Measured result and scope

- Local source preparation: published driver base `30dfdb32957215b4c59da3ac763ba14ab0c482d0`, pinned RenkuOS dependency `4799bb5a188c4bb6bbef967287640cc21a061a0d`, plus existing patches through 0122. Independent checks matched all 969 manifest payload hashes in the bundle and staged tar, published preparation tools and bundle Makefile. This does not prove continuously immutable build inputs.
- Native VM build: platform check and both driver/accelerant compile/link targets exited 0 using a 60-second make bound. Independent guest/workspace/evidence artifact bytes and expected ELF64 x86-64 entry points matched. The build used installed VM SDK/private headers and emitted a clock-skew warning; reproducibility and runtime behavior were not established.
- Software fixtures: independent native C++ transition fixture, including two-thread admission, exited 0. Host/software fixture success does not exercise actual kernel hooks, gLock, completion or resource release. Actual-entry-point race and cleanup tests remain missing.
- Live reachability: no cycle 77 physical observation; cycle 76 endpoint identity is historical and does not establish current GPU health.
- Image boot: no cycle 77 installation, candidate image boot or loaded-byte verification.
- Verified GPU execution: none. No physical GPU test or DMA-safety proof; uncertain backing, accepted baseline and known-good recovery artifacts remain preserved.

## Exact source and artifact hashes

| Item | Bytes | SHA-256 |
| --- | ---: | --- |
| `intel_extreme.driver` | 260192 | `bb19be16bc39180cc63ae488b50710f60fe14975e9a874ec16f6fd3eee577fb5` |
| `intel_extreme.accelerant` | 1148768 | `73d6eaadb9a1230b8b9f2cafa7101205b6b24de4756df73e6544074b5e642340` |
| Prepared/guest `device.cpp` | — | `5d52e1caf7f8ecfec3b4e64dfe00b659faa1e30360b31faa9b289c4616532224` |
| Patch 0122 | — | `6d222259ee61e5f46229fbd064b83788c1fd0665e73f7203853ec8a12c5b028b` |

## Unresolved issue and next action

Patch 0122 holds gLock across ioctl allocation, copyout/rollback and free against close/free. It does not connect request_lifetime.h to production admission, completion or cleanup. Lower-layer lock order, blocking user copies, cookie lifetime and unload synchronization remain unverified; serialization can increase contention.

Atomically bind backing to owner/generation on admission and enforce that binding on release. Validate completion in the kernel, release backing before IDLE, and retain retryable ownership on cleanup failure, including copyout rollback. Preserve uncertain DMA on timeout/close; do not allow userspace retirement. Test actual entry points for competing admission, admission/free, rollback, close/teardown, stale/duplicate completion, timeout, failed cleanup/retry and the synchronization concerns above. Prepare and natively rebuild both artifacts with bounded commands and raw statuses, then hand off to the designated independent verifier before a reviewed physical experiment.

## Selected evidence

Workspace-relative references; evidence and binaries are not copied into this report:

- `evidence/native-cycle-77/integration-handoff.json`
- `evidence/native-cycle-77/independent-verification.md`
- `evidence/native-cycle-77/native-build.json`
- `evidence/native-cycle-77/native-build.log`
- `evidence/native-cycle-77/independent-transition-test.log`
- `driver-development/project/patches/0122-serialize-device-ioctl.patch`
- `cycle-ledger.json`
