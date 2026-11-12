# Cycle 88 — native build and 62-case fixture recovery

Chain ID: `cycle-197a451f-bbad-42da-aed9-505fd3a39408`. Integrator outcome: `advance`; independent verification remains outstanding. The native-driver-baseline milestone and overall GPU objective are incomplete.

Recovery reports native compilation of both binaries with guest/helper exit 0 using `timeout 75 make -j4 check-platform build/intel_extreme.driver build/intel_extreme.accelerant`. The bounded regression build/run also returned guest/helper exit 0: 62 cases, 0 failures. These exercise production translation units with mocked OS boundaries and release backends; they establish neither GPU operation nor concurrency correctness.

Local preparation: candidate-cycle-88 retains cycle 87's production cleanup change: completion-only release validates cleanup state and allocation ownership/generations/backing under the lock, retains binding on backend failure, and links the extracted memory_release implementation. Cycle 88 changes only the fixture: reset the retained request before the next open and reject failed/null cookies. Fresh independent preparation from the pinned sources and reconciliation of all candidate deltas are outstanding.

Failed/incomplete work: cycle 87's expanded fixture timed out after a failed open/null-cookie path; recovery reports terminating the orphan under the canonical VM lock and observing its absence. Extraction/make clock skew was recorded. The hashes/process check returned 1 because the trailing process grep found no match, according to recovery; retained host/guest artifact hashes matched. Successful compilation does not prove whole-binary reproducibility.

Live reachability: recovery reports read-only N100 SSH success and a RenkuOS desktop on the console. This is reachability evidence only. Candidate image boot/deployment was not attempted; loaded candidate bytes are unproved. No verified GPU execution occurred. Current lease ownership and DMA safety remain unknown; uncertain backing is retained. This publisher performed no device operations.

## Exact identities

Pinned published driver commit: `30dfdb32957215b4c59da3ac763ba14ab0c482d0`. Pinned RenkuOS dependency: `4799bb5a188c4bb6bbef967287640cc21a061a0d`.

Candidate archive SHA-256: `9883a3a7e1883ad3d0c8618e1a64a7ee3f4c1eeef76fe1a6c9571109641d72d5`.

| Retained artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `intel_extreme.driver` | 324968 | `7cd4ec9d81fe079fcc65c8218db373c2886154ce240875a95d4c77f434d673a9` |
| `intel_extreme.accelerant` | 1236152 | `4328593a7b76fdaf76d61da61257fc2ca546957be2019c8382535d0c3b211c52` |

Candidate source SHA-256 (paths relative to `driver-development/candidate-cycle-88/`):

- `Makefile`: `51a58bdfd5f3cdfc76508938f16f11c757be017233cbc32b60ae65f1b911447c`
- `tests/cleanup-regression.cpp`: `89472e4d9e0d7bff90b04cad6ae1892155fcd3fb6dbc1494a25dad5faadadd0f`
- `source/src/add-ons/kernel/drivers/graphics/intel_extreme/device.cpp`: `bdeec114799ef346fa3acd86b478b857845d241a7182c3ff871d170a06f992d1`
- `source/src/add-ons/kernel/drivers/graphics/intel_extreme/driver.cpp`: `6018dea0deb430d4ec4e11a411f7f7b45a4a3ded1abb9f21e312d03709f53aa1`
- `source/src/add-ons/kernel/drivers/graphics/intel_extreme/driver.h`: `e5f49fc29ad9743be961c362b7822359f4ffa949de1d5c01dda8b70d1724fbd5`
- `source/src/add-ons/kernel/drivers/graphics/intel_extreme/intel_extreme.cpp`: `80a204d022f8bf872554d7c709e24df22ce93debec0b5dd509852a23e3c3a5b1`
- `source/src/add-ons/kernel/drivers/graphics/intel_extreme/memory_release.cpp`: `3b93530b0674bbe329c49dbce241245fa01fe4aac591c02c9257d6536ae79c1c`

## Unresolved issue and next action

Independent verification and reproducibility of the pinned baseline remain unestablished. Preserve the integrator’s next action verbatim:

Configured independent verifier: stage candidate-cycle-88 in a fresh VM directory under canonical test.lock, run timeout 75 make -j4 check-platform build/intel_extreme.driver build/intel_extreme.accelerant, then timeout 25 make cleanup-regression and timeout 8 ./cleanup-regression; retain raw output, guest/helper exits and artifact bytes; independently prepare the pinned published sources and account for candidate deltas.

## Selected evidence

Workspace-relative references; evidence and binaries are retained locally, not included in this report commit:

- `cycle-ledger.json`
- `evidence/native-cycle-88/integration-handoff.json`
- `evidence/native-cycle-88/publisher-handoff.json`
- `evidence/native-cycle-88/recovery-handoff.md`
- `evidence/native-cycle-88/build.log`
- `evidence/native-cycle-88/regression.log`
- `evidence/native-cycle-88/host-hashes.log`
- `evidence/native-cycle-87/cleanup-repair.patch`
- `evidence/native-cycle-88/test-fix.patch`
