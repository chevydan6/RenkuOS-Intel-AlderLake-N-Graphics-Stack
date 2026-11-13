<!-- SPDX-License-Identifier: MIT -->
# Host bitmap/bit-operation checks

These complement physical-bitmap-001; they are host execution results. The
original complete find_bit.c/hweight.c and completion/swait algorithms run under
ASan/UBSan with pthread/POSIX services, modeled IRQ state and an independent
bit-by-bit search oracle. Final result: 46362 checks, 12 phases and 24 waiters,
plus actual host signal interruption. Lost completion-count and ignored search
offset mutations both abort as required. Invalid GENMASK, mixed-type min and
BUILD_BUG_ON(1) fail compilation; the positive helper compiles.

Compiler warnings are retained in build.log: the original private nth helper
has unsafe direct zero-size entry. Its public wrappers guard that path; only the
source's maybe-uninitialized warning is downgraded from Werror. See the native
README for the exact limits. The initial strict build failure is also retained.

Twenty-two first-header controls compile against host and actual RenkuOS headers;
commands and results are in header-controls/. Reproduce using tests/run-bitops-headers.py after target-objects. The script is the source for generated controls.
IRQ model checks compile the actual native adapters with target headers and
ASan/UBSan, substituting modeled native services; the premature-enable mutation
exits 90. VMA access/revocation and DMA API/rollback mutation tests also pass.
Native hardware/concurrency is established only to the extent separately recorded
in physical-bitmap-001. Older component and target-probe regression logs are there.
