# A30 reset helpers — software validation only

Observed 2026-11-07; source identities are recorded in `inputs.json`.

* Reset-only helper: 431 cases passed on Linux with ASan/UBSan and on the N100
  CPU with native GCC 13.3.0; native exit 0. Input hashes printed by the native
  command matched workstation files. Native run was visible through NanoKVM.
* Domain planner: 4,101 cases passed with host ASan/UBSan, including all 4,096
  VDBOX/VEBOX fuse-field combinations and identity/revision/all-ones rejection.
  This separate planner has not yet been compiled natively.

Host compile options: `g++ -std=c++17 -O2 -g -Wall -Wextra -Werror
-fsanitize=address,undefined -fno-omit-frame-pointer -Iworkstreams/gpu-next`.
Sources: `tests/bcs-reset-test.cpp` and `tests/reset-domains-test.cpp`.
Native reset test options: `g++ -std=c++17 -O2 -Wall -Wextra -Werror -I.`.

The expected store trace is independently specified in the test. Tests inject
each read/write failure, all-ones reads, ownership loss, clock faults, phase
timeouts, frozen clocks, conflicting reset state, pending wake and second use.
Failed stores are modeled as having changed hardware before returning false.

No kernel reset adapter, physical reset, DMA or GPU command ran. Reference
identities in `references.json` are research sources, not linked Linux code.
See `research/n100-submission/BCS-RESET-DESIGN.md` for source-derived constraints
and the remaining all-domain forcewake integration.
