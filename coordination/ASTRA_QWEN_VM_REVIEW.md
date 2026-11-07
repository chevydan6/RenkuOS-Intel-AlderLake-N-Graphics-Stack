# Astra independent review of Qwen PPGTT VM validation

2026-11-07. Reviewed `coordination/QWEN_VM_VALIDATION.md` and the actual
artifacts in `/boot/home/test-work/qwen-ppgtt-001`.

**Accepted as native VM software-regression evidence.** Independently confirmed:

* Both headers and both test sources match current project files byte-for-byte.
* Original ELF64/x86-64 binaries have the reported sizes:8120 and23480 bytes.
* Original binaries passed2150 PPGTT encoding/range and69 owned-tree cases,
  each exit0.
* Rebuilt both from those exact sources in the separate guest workspace
  `/boot/home/test-work/astra-qwen-vm-review-001`, using the reported
  `g++ -std=c++17 -O2 -Wall -Wextra -Werror -I.` flags. Builds and reruns passed,
  all six build/run exit records are0.
* Both rebuilt binaries are byte-identical to Qwen's originals.
* Observed hostname `renkuos-tests`, hrev60072+518 and native GCC13.3.0.
  Kernel, `_KERNEL_` and version-glue hashes exactly match the A34 baseline.

The original report/workspace had no saved build/run logs, exit records or
source hashes. Independent replay and reproducible rebuild now supply those
missing records. This corroborates the reported results; it cannot reconstruct
the original invocation history. The report's phrase "offline host validation"
is imprecise: these are **native guest userspace tests**, not kernel fixtures.

Original/rebuilt SHA-256:

* PPGTT: `c0805a3b439cd1b46294cac9a4237724e21dc9c3bab996c799bf0a72a040ea64`
* Tree: `bde7c29e7ca933328f8be7e525e6fc2d9244839ec44c0346604c923ae5e0246f`

Evidence: `tests/evidence/qwen-ppgtt-vm-review-001/`; collected originals and
rebuilt artifacts: `build/astra-qwen-vm-review-001/`.
Builds and runs used `vm.py run` and its transaction lock. A later collection
attempt found another transaction holding the lock; no retry/reset/rollback
was performed. Read-only SSH then collected this review's completed files.
Qwen's files remained unchanged; no physical GPU operation occurred.

No further Qwen action is required. This closes the requested review and is
not a gate on physical development. GPU DMA/cache behavior, kernel integration,
hardware submission and Vulkan are not established by these results.
