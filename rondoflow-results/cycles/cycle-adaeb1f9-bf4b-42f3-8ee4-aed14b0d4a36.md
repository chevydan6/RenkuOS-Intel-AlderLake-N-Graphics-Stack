# Cycle 89: verified candidate; owner-exit cleanup repair required

Chain: `cycle-adaeb1f9-bf4b-42f3-8ee4-aed14b0d4a36`.

Published baseline independently verified separately; modified candidate native build exit 0, regression 62/62. Owner exit followed by confirmed completion leaves other unpublished allocations retained and idle teardown blocked until another lifecycle call. REPAIR_REQUIRED.

Pinned source `30dfdb32957215b4c59da3ac763ba14ab0c482d0`, RenkuOS `4799bb5a188c4bb6bbef967287640cc21a061a0d`. The unchanged baseline receipt is `driver-development/baseline-verification.json`; it records independent native verification in cycle 75. Cycle 89 independently reconstructed the published bundle, accounted for 15 candidate payload deltas and two manifests, and built the modified candidate in a fresh VM directory. Both candidate targets built successfully (guest/transport exit 0). Cleanup regression: 62 cases, zero failures, exit 0. Debug-stripped candidate bytes match cycle 88; whole binary hashes differ due to compilation directories.

| Artifact | Published baseline SHA-256 | Modified candidate SHA-256 |
| --- | --- | --- |
| intel_extreme.driver | `3b0f017fefa6979ba3c70cd785848fbf88c36e4f818cd12272c4927748123080` | `0acac7647e803e1fb45bb109e9a5203599ac3fc00c590006e12264d646ba163d` |
| intel_extreme.accelerant | `31a8ab0446124c1430afc719f0f37ec98e35071f03b1b62e1c3b7d7ffe91becc` | `336aefaaec4131c3c537ecf4fbcb3cb33332f5926336e4039b47ed8bda2a56d6` |

Production cleanup validates owner, request generation, allocation generation, backing address and linked identity. Failed backend release preserves CLEANUP binding; ordinary free and close retain uncertain DMA. Remaining defect at `driver-development/candidate-cycle-89/source/src/add-ons/kernel/drivers/graphics/intel_extreme/device.cpp:239`: completion after owner exit frees the bound allocation but fails to retry other unpublished allocations blocked by request retention, preventing idle teardown.

Next executable unit: Add a two-allocation owner-exit regression for both release backends in candidate-cycle-89/tests/cleanup-regression.cpp; correct device.cpp intel_complete_request to drain eligible unpublished allocations after successful completion, retain failed releases, then run idle teardown. Implementer builds; designated independent verifier rebuilds with timeout 75 make -j4 check-platform build/intel_extreme.driver build/intel_extreme.accelerant, timeout 25 make cleanup-regression, and timeout 8 ./cleanup-regression under canonical VM test.lock, retaining raw logs, actual exits and artifact bytes.

Evidence: `evidence/native-cycle-89/independent-verifier-result.json`, `verifier-build.log`, `verifier-regression-build.log`, `verifier-regression.log`, `verifier-source-check.log`, `verifier-stripped.log`; baseline `evidence/native-cycle-75/independent-vm.stdout.log` and `driver-development/baseline-verification.json`.

Fixture locks and teardown are mocked. No concurrency or physical GPU behavior established; `intel_complete_request` has no production caller. No hardware operation or deployment performed. Physical admission lacks an exact reviewed bounded procedure, current canonical lease admission, installed compatibility checks and identified known-good rollback. Preserve uncertain DMA, lease, source and boot/display artifacts. These constraints do not block native repair builds.

Previous publication: https://github.com/chevydan6/haiku-intel-gpu/blob/7f299fbdc349d01a7225e727bcec257b7ce77ae3/rondoflow-results/cycles/cycle-197a451f-bbad-42da-aed9-505fd3a39408.md. REPAIR_REQUIRED; no overall GPU success.
