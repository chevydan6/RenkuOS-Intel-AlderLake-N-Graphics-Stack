<!-- SPDX-License-Identifier: MIT -->
# Submission design review workstream

This is a self-review checklist, not independent project approval. Implementation
must not close these items solely because a host fixture is green.

- [x] Identify physical target and preserve build 057 display baseline.
- [x] Separate MI execution proof from BLT/rendering and Mesa integration.
- [x] Pin inspected Linux sources with hashes and direct upstream URLs.
- [x] Record ADL-N GuC firmware exception; do not assume ADL-P binary selection.
- [x] Define request states, timeout quarantine and no-free-before-retirement rule.
- [x] Require fixed kernel batches and negative CPU-verifier controls.
- [ ] Independent review of candidate execlists path on exact N100 stepping.
- [ ] Resolve cleanup-only GART record reuse contract for every client.
- [ ] Resolve device/module unload barrier with in-flight/quarantined DMA.
- [ ] Resolve hidden partial-bind ownership and bounded recovery.
- [ ] Physical validation of newer memory code, preserving recovery image.
- [ ] Verify engine inventory, GT forcewake, power, reset and interrupt rules.
- [ ] Review complete context/PPGTT initialization and cache/TLB ordering.
- [ ] Retrieve and review MOCS/PAT reference; initial retrieval was rate-limited.
- [ ] Produce register/packet field provenance with applicable stepping workarounds.
- [ ] Demonstrate reset-only recovery and retained-resource handling on hardware.
- [ ] Implement and independently verify physical MI-store experiment.

Review finding: automatic GART reopen cleanup treats failed unmap as a teardown
request; this is not yet an established contract for all clients. Do not deploy
it or build submission ownership on that assumption without caller review.
Review finding: existing cache flushes and GGTT tests are not proof of GPU idle,
PPGTT correctness or CPU visibility of a completed request.
Review finding: one successful MI store is not safe object retirement. Require
ordered completion and inactive context before releasing any backing.

Current recommendation: proceed with owned-memory lifetime design and read-only
GT inventory implementation; keep workload publication disabled until all
pre-execution gates in DESIGN.md have concrete evidence.
