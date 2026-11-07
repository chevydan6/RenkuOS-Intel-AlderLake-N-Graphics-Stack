# A37 initial BCS context-image software evidence

715 host ASan/UBSan and715 native GCC13.3 VM cases passed:681 full16KiB
comparisons against extracted pinned Linux routines and34 rejection/guard
checks. Native inputs were hash-checked before compilation; collected executable
hash matches the guest log. Current source/test/generator hashes match host input
records. Native ELF SHA-256:
`b4d319c25db2a0dd45879e2943a6fcca37bfdb28a2f69f586e96a95d852b86ad`.

Candidate: `workstreams/gpu-next/IntelN100ContextImage.h`.
Test and extraction runner: `tests/n100-context-image-test.cpp`,
`tests/context-image-host.py`.
Field-by-field design: `research/n100-submission/CONTEXT-IMAGE-DESIGN.md`.
Pinned references: `build/a37-context-image-research-001/`.
Final host build: `build/a37-context-image-host-004/`.
Native build: `build/a37-context-image-native-001/`; guest workspace
`/boot/home/test-work/a37-context-image-native-001/`.

The source-derived oracle caught a saved STOP_RING mask error (bit9 instead of
bit8); `detected-stop-mask-mismatch.log` preserves that finding. Corrected to
0x01000000 before native build or hardware use. The final complete image matches
the extracted routines for the fixed N100 profile, including workaround pages.

Reference code uses target-profile/address shims, not a full Linux GPU driver.
This is CPU construction evidence only. No GPU publication, context mapping,
DMA retirement, complete stepping workaround or execution result is claimed.
VM build used its transaction lock; no physical driver changes or VM resets.
