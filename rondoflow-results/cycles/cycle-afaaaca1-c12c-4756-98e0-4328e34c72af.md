# Cycle 92 — repair required

Chain: `cycle-afaaaca1-c12c-4756-98e0-4328e34c72af`. Integrator outcome: **repair**; driver work remains open.

Independent native driver and accelerant builds passed (guest and host exit 0), and cleanup regression passed **128 checks, 0 failures** (guest and host exit 0). The implementation build reused the accelerant; the independent build compiled and linked both artifacts. The separately verified pinned baseline remains distinct from this candidate.

The lifetime regression compiled but failed its assertion at `request-lifetime-test.cpp:13`: it expects retention before publication. Guest exit was unobserved; helper host exit was 1 after timeout. The first helper invocation also had an argument-order error. The verifier removed the stopped test and its timeout and confirmed cleanup. Passing cleanup checks do not override this failure.

Scope: local source preparation and native VM software builds/tests only. Cleanup fixtures mock OS/hardware boundaries and seed UNCERTAIN state; they do not exercise a real production publication transition. No new live N100 reachability measurement, candidate image boot, deployment, or verified GPU execution occurred. Historical reachability does not establish current device state. Production lifetime readiness and deployment readiness are not established; deployment is not authorized. Hardware custody and uncertain DMA backing remain preserved.

Unresolved: deterministic admission/close and copyout/free concurrency coverage, rollback, stale owners, binding mismatches and exactly-once cleanup; callback cookie lifetime protection; a real production `begin_publication` caller holding device/context and driver references before dereference and throughout publication; close/failure/timeout coverage. Original and rebuilt binaries differ in full bytes. The verifier reports debug-stripped comparison exit 0 for both pairs; full-byte reproducibility remains unestablished.

Next action: Repair driver-development/candidate-cycle-92/tests/request-lifetime-test.cpp: reject pre-publication retention/completion, bind valid allocation/backing and call begin_publication before retention in both sequences; replace assertion aborts with diagnostics and nonzero exits, and cover pre-publication cancellation through the production close hook.

Then use the designated independent verifier for the revised source and bounded native build/test units specified in `evidence/native-cycle-92/integrator-result.json`; resolve the remaining concurrency, production caller and reproducibility gaps separately.

Source identity: published driver commit `30dfdb32957215b4c59da3ac763ba14ab0c482d0`, dependency commit `4799bb5a188c4bb6bbef967287640cc21a061a0d`. The verifier matched all 975 regular files in the source snapshot; manifest and retained baseline provenance do not establish a fresh reconstruction of every inherited delta. Recorded candidate SHA-256 values below are relative to `driver-development/candidate-cycle-92/`:

- `source/src/add-ons/kernel/drivers/graphics/intel_extreme/request_lifetime.h`: `680f0bfd8c5621703e0b1a79b09c5e8d85813407b6a9ee5178b9ff83cef6a259`
- `source/src/add-ons/kernel/drivers/graphics/intel_extreme/device.cpp`: `38ff3ac0a1dcca91a35570d494af7b27df0b6d2d1961c5848616926903db6891`
- `tests/cleanup-regression.cpp`: `ae3014d57899aed636e09d5fac7fd892dcafb4ef08844f929aaed42053218996`

Recorded artifact SHA-256 (original → independent rebuild):

- `intel_extreme.driver`: `ab8f30d474bf243d8a83132558994c30d726ee8182a2be906035d22e9b273fd1` → `b7f7b97fc22fdc3253a5bed7cdde822d0c15e8fdf657a8823537cbb68f22cc7e`.
- `intel_extreme.accelerant`: `b9ebe615cf2a717789e63dd4eeca5ef3e0c2ce5d8f23bb4e98a82cde364eafd7` → `95602a42c3685505461a7db114386350e3e2e9f30ee28b960fe0611c54fe3ce8`.

Selected workspace-relative evidence (references only; raw logs and binaries are not published):

- `cycle-ledger.json`
- `evidence/native-cycle-92/integrator-result.json`
- `evidence/native-cycle-92/publisher-handoff.json`
- `evidence/native-cycle-92/publication-input.md`
- `evidence/native-cycle-92/independent-review.txt`
- `evidence/native-cycle-92/independent-status.json`
- `evidence/native-cycle-92/independent-lifetime-test-retry.log`
- `evidence/native-cycle-92/independent-process-cleanup.log`
