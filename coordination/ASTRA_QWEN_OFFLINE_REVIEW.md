# Astra independent review of Qwen offline validation

Reviewed `QWEN_OFFLINE_VALIDATION.md` against the original artifacts in
`build/qwen-offline-validation-001/` on 2026-11-07.

* All nine reported source/binary SHA-256 hashes match the actual files.
* All six source/header files match the current project byte-for-byte.
* All three binaries link ASan and UBSan.
* Independently executed all three: 32 nonce, 2150 PPGTT encoding/range and
  69 PPGTT tree cases passed, each exit 0 with empty stderr.

Captured original-artifact results: `build/astra-qwen-original-review-001/`
(`review.json` and per-program logs). The original report did not include
complete compile/run logs and exit records; its stated results are now
independently corroborated. Earlier Astra rebuilds of the identical sources
also passed with recorded C++17 warnings-as-errors and ASan/UBSan commands:
`tests/evidence/offline-submission-review-001/`.

Accepted as offline regression evidence for future submission work. This does
not establish physical GPU execution, kernel integration or Vulkan support.

## v002 completion-report recheck — 2026-11-07

Independently checked `QWEN_OFFLINE_VALIDATION_002.md` and
`build/qwen-offline-validation-002/` against the existing v002-specific review
in `tests/evidence/offline-submission-review-001/review.json`.

All nine reported hashes match both the actual artifacts and prior independent
review. All six sources still match the current project. All three binaries
link ASan/UBSan. Prior original-binary execution and independent rebuild/run
records have exit0 and matching32/2150/69 outputs. These are unchanged artifacts;
no redundant executions or builds were needed for this recheck.

Accepted as host-only regression evidence. The claim of comprehensive original
capture is overstated: Qwen's three compile logs are empty and its directory
still has no separate run/exit records. The existing independent review supplies
those records. Sanitizer passes mean no findings in the exercised cases, not
a proof of absence of bugs. No additional Qwen action is required.

Fresh hash/linkage recheck: `build/astra-qwen-offline-recheck-002/review.json`.
