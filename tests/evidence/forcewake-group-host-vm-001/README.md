# Multi-domain helper software evidence — 2026-11-07

964 cases passed with host C++17 `-O2 -Wall -Wextra -Werror` and ASan/UBSan,
then with native GCC 13.3.0 on the local RenkuOS VM (same flags without
sanitizers). `vm-native.log` records hostname `renkuos-tests`, kernel
hrev60072+518, exact input hashes verified before compilation and the pass line.
`inputs.json` identifies the tested sources. The VM run used
`/home/dan/ai/vms/renkuos/vm.py run`, holding its separate transaction lock.

Test command uses `tests/forcewake-group-test.cpp` with include directory
`workstreams/gpu-next`; see `research/n100-submission/FORCEWAKE-GROUP-DESIGN.md`
for failure coverage and the corrected input-staging issue from the earlier run.
No N100 GPU is passed into this VM. No physical all-domain handshake or reset
is claimed by this evidence.
