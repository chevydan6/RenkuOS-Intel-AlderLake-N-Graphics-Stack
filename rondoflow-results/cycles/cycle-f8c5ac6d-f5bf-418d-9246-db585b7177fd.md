# Cycle 84 — repair; production verification incomplete

Chain ID: `cycle-f8c5ac6d-f5bf-418d-9246-db585b7177fd`. Integrator outcome: **repair**, preserved.

Local source preparation repaired legacy ABI layouts, added capability query and versioned allocation/free operations, enforced cookie ownership and legacy/versioned isolation, initialized requests/outputs, limited capability fallback to `B_DEV_INVALID_IOCTL`, and captured open initialization status under `gLock` with a null initial output cookie. Normal preparation exited 0. Native Haiku driver and accelerant builds each exited 0 and both binaries were retrieved. Clock-skew warnings occurred from transported timestamps; the fresh build directory contained no old objects.

Independent verification inspected candidate hashes, ELF64 x86-64 Haiku ABI and required exports. Two clean-path rebuilds from the saved bundle each exited 0. Native accelerant hook lookup passed **185 checks, 0 failures**. These checks do not execute complete production device/driver paths. Host fixtures and boundary mocks do not establish physical GPU behavior; earlier baseline verification does not verify these candidate bytes.

Cross-path whole-file hashes differ for both binaries. Corresponding binaries match byte-for-byte after stripping debug information; neither has a `.note.gnu.build-id` section. Debug compilation paths differ, but this does not prove every debug-byte difference is solely the path. **Whole-file reproducibility remains unestablished.** Independent reprepare/comparison against pinned sources is outstanding despite matching manifest/archive payloads.

Complete production `device.cpp` and `driver.cpp` executable tests remain outstanding: old/new ABI pairings, malformed queries/lengths and copy faults, ownership isolation, controlled failed/successful open interleaving, binding/completion validation and stale/duplicate completions, completion after cookie release, cleanup failure/retry and retained-state reattachment, exactly-once teardown, image pin/release, allocation failure/timeout retention, close/free/unload concurrency, and clone/shared-backing lifetime. Production caller/submission wiring and trusted completion binding remain unresolved.

A reviewer continuation reported exit 1 with `OSError: [Errno 30] Read-only file system` opening canonical `test.lock`; no guest command ran and that launch saved no raw preflight log. This EROFS is specific to the read-only reviewer launch and does not establish a configured-verifier attachment failure.

Live N100 reachability was not tested this cycle; prior endpoint identity is historical only and establishes neither GPU health nor DMA safety. No candidate image boot, physical installation, GPU execution, hardware recovery or restart occurred. Historical custody, known-good media and uncertain DMA remain preserved.

Next action: configured verifier `85a01cd9-258a-475e-ad8c-1f2994f15775` runs the existing canonical VM helper with a 15-second `uname -a` preflight, then executes complete production-unit tests with only OS/hardware boundaries mocked, including the controlled failing-opener/successful-opener interleaving. Repair attachment access only if that configured verifier reproduces the failure. Independently reprepare and compare pinned source derivation; record exact hashes and test results. No GPU result is claimed.

## Source and artifact identifiers

Pinned published driver commit: `30dfdb32957215b4c59da3ac763ba14ab0c482d0`. Pinned RenkuOS dependency: `4799bb5a188c4bb6bbef967287640cc21a061a0d`; no full operating-system rebuild is established.

Patch: `driver-development/project/patches/0125-request-admission.patch`, SHA-256 `46c07fad9c707cd448fe90fd2ed6a09442f62e30d1fe0a46d0312f4eb929daa1`.

Prepared source SHA-256 (paths relative to the prepared source tree):

- `source/headers/private/graphics/intel_extreme/intel_extreme.h`: `9c776b1b0563238de50e999919aab666789b06bd5e6f018035f2dc70943d0edb`
- `source/src/add-ons/accelerants/intel_extreme/memory.cpp`: `7f3b1fe9a2fa441343e2261419b7350be55a5900b5e425d66a86302a751f2747`
- `source/src/add-ons/kernel/drivers/graphics/intel_extreme/driver.cpp`: `6018dea0deb430d4ec4e11a411f7f7b45a4a3ded1abb9f21e312d03709f53aa1`
- `source/src/add-ons/kernel/drivers/graphics/intel_extreme/allocation_ownership.h`: `c4d67900d482be748e4b7fcd4b203e852df63274b919841998713eaf5238c213`
- `source/src/add-ons/kernel/drivers/graphics/intel_extreme/device.cpp`: `8cce0ecc8bc646505ec72d23cf723f81775087e96703274350d719a908e24102`

Retrieved native artifacts (SHA-256):

- `driver-development/artifacts/cycle-84/intel_extreme.driver` (309592 bytes): `d6e55da59cf7e9a6e8571092e96dd117b1df27aab5d029b9fec635588ea2ce63`
- `driver-development/artifacts/cycle-84/intel_extreme.accelerant` (1236152 bytes): `e9d2d600d8bfe0bdc1f290f013317931a8903892e1cb2bd62cf3ab752d61fefa`

Corresponding stripped cross-path SHA-256: driver `9fe063392959a340b5cf8f7b334cc01a83b2fc715f958688220d7bef2ba8d926`; accelerant `6a68be579c6743e8bf8239cd8029af342df6cdcc45b1bf3344b3236832834725`.

## Selected evidence

Workspace-relative references; evidence remains in the development workspace, not attached as raw logs or binaries:

- `cycle-ledger.json`
- `evidence/native-cycle-84/native-build.json`
- `evidence/native-cycle-84/independent-verification.md`
- `evidence/native-cycle-84/verifier-hooks.log` and `verifier-hooks.status`
- `evidence/native-cycle-84/verifier-binary-comparison.log` and `verifier-binary-comparison.status`
- `evidence/native-cycle-84/integration-handoff.json`
- `evidence/native-cycle-84/publisher-handoff.json`
