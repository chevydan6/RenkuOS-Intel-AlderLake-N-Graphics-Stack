# Cycle 98 — saved-output signature repair accepted

Chain: `cycle-0bbfcc53-ff5c-49b3-9dfd-07606aad5c48`. Integrator outcome: **advance**. Input results are generation 96, chain `cycle-c6b56483-1eda-45af-8ace-212f6911c605`.

Generation-98 independent comparison exited 0 with empty stderr: six ordered-prefix counts matched **3, 6, 2, 2, 21, 3**, and exact failure multisets matched **39 and 56**. Each negative control recorded exit 1 as expected. Saved-output signature verification is accepted and closed; these expected control failures are preserved, not omitted.

Generation-96 implementation and independent native builds exited 0. The independent cleanup fixture passed **247 checks, 0 failures**; five-artifact validation exited 0. Existing coverage includes actual cookie/node destruction tracking and initialized PCI/GART teardown contention. No new build or source implementation occurred in generation 98.

Scope and limits:

- Local prepared candidate: `driver-development/candidate-cycle-92`; separately verified native baseline remains preserved. Published source pin: `30dfdb32957215b4c59da3ac763ba14ab0c482d0`; dependency pin: `4799bb5a188c4bb6bbef967287640cc21a061a0d`. Installed SDK build; no full OS rebuild.
- Software fixtures and saved host outputs establish software checks only. Full guest `make test` remained incomplete, exit 2, because guest Python was unavailable. Saved-output comparison does not complete the guest suite.
- Independent artifacts differ from implementation artifacts; byte-identical reproducibility remains unproven. The cleanup fixture manually assigns UNCERTAIN and establishes neither DMA completion nor GPU operation.
- Live reachability: no new measurement in this cycle; historical observations are not current device evidence. Image boot/deployment: none this cycle, loaded bytes and lineage unproved. Verified GPU execution: none. No device admission or physical experiment approval; current lease ownership and DMA safety remain unknown, with uncertain backing preserved.

Unresolved production issue: validated `intel_submit_request()` requests still return `B_NOT_SUPPORTED`; no hardware publication backend exists. `REPAIR_REQUIRED` means no device admission, not reopened signature verification.

Next action (future work, not implemented): add an internal publication-result boundary in `driver-development/candidate-cycle-92/source/src/add-ons/kernel/drivers/graphics/intel_extreme/device.cpp`, distinguishing rejection before publication from possibly published failure; retain uncertain backing and reject unsupported hardware. Extend `tests/cleanup-regression.cpp` for both outcomes, then compile and run the native cleanup fixture through the existing VM transaction helper in a logged unit of at most 90 seconds for independent verification. Physical backend support and live experiments remain separate.

SHA-256 identities (recorded artifacts; binaries excluded from publication):

| Scope | Artifact | SHA-256 |
| --- | --- | --- |
| Verified baseline | intel_extreme.driver | `3b0f017fefa6979ba3c70cd785848fbf88c36e4f818cd12272c4927748123080` |
| Verified baseline | intel_extreme.accelerant | `31a8ab0446124c1430afc719f0f37ec98e35071f03b1b62e1c3b7d7ffe91becc` |
| Generation-96 implementation | intel_extreme.driver | `411d395312a5493d5caa27d58c1bf121262e17e322466fb343c18ebf824ee472` |
| Generation-96 implementation | intel_extreme.accelerant | `6d35e50b5920c997760e0d807c3d77af223b4361f8193c8bcf8fb86eaf2f8ec1` |
| Generation-96 implementation | cleanup-regression | `7c5012058c89bd93b2bd6e71345888b67e8c70ada68e34030dd166671436eb32` |
| Generation-96 independent | intel_extreme.driver | `03038971e3530dcd8e217b8f21d73aa928c7448cc8221dfb3f6139b072d75d18` |
| Generation-96 independent | intel_extreme.accelerant | `15828bd84dacf2bb746d175608156fc3f4610d4bdc924f318f62a2e3975cc1da` |
| Generation-96 independent | intel_gart | `a4b6d2d6c2503523a72fa0205b540a6b8aa110d120044c2f559219e2ab3d74d7` |
| Generation-96 independent | agp_gart | `5b8e8d15bc384bc795824cfd2bd23324637b3c884d36b78def77acb408496060` |
| Generation-96 independent | intel_memory_owner | `a45c3b75961eebab43f95e8bb0e46aa87e704f554be311ab6c79539fa5349a58` |

Source/control identity: `driver-development/candidate-cycle-92/tools/regression-control.py`, SHA-256 `95c44c1ed1802b53b187c3adeaae9dccbc8d80e10337e5c6dbc5086f602bd461`.

Selected evidence references are workspace-relative and are not copied raw into this report:

- `evidence/native-cycle-98/publisher-handoff.json`
- `evidence/native-cycle-98/independent-verification.json`
- `evidence/native-cycle-98/signature-comparison.stdout.log`
- `evidence/native-cycle-96/independent/regression.log`
- `evidence/native-cycle-96/independent/artifact-validation-cd6n4nsn/receipt.json`
- `driver-development/baseline-verification.json`
- `cycle-ledger.json`

Generation-98 comparison stdout SHA-256: `3e4ad7b895b652c07a6e292bea84486738656836fa83a2f14e7bb8adc72d1cee`.

Previous publication: cycle 97, commit `94d363f8ddb94c6d517fcd40918c7c4c722e691b`; its report is unchanged.
