<!-- SPDX-License-Identifier: MIT -->
# CPU adapter host/model checks

These results complement physical-cpumask-001 and do not claim native hardware
execution. completionCPU runs original completion/swait/find/hweight and mask
helpers under ASan/UBSan with pthread/POSIX services, modeled IRQs and a published
4-CPU/2-online snapshot (mask 0x5). It passes 46523 checks and previous signal cases.
Lost completion-count and ignored-offset mutations abort; invalid GENMASK,
BUILD_BUG and mixed-min compile controls reject their intended errors.

cpuModel compiles the actual native CPU query adapter and common mask state with
real RenkuOS declarations, ASan/UBSan and modeled CPU queries. It checks counts 0,
1, 4, 64 and oversized 65, sparse enabled IDs, CPU 63, invalid masks, query failure,
count changes during startup, duplicate startup and reset/reinitialization.
Incorrectly admitting disabled CPUs exits 90 as required by the mutation control.
The first negative-control harness used an incorrect expected exit code; the
stable 90 control and final test pass. That earlier attempt remains outside Git.

Forty-four first/repeated-header checks pass with host and real target headers,
including glibc limits bootstrap, Linux namespace selection and early config.
Commands/results are in header-controls; tests/run-bitops-headers.py reproduces
them. Other directories retain DMA, VMA and native-IRQ model sanitizer/mutation
regressions. Component allocator/refcount/ww/emitter and target-header probe logs
are in the physical evidence directory. Original find_bit warnings are retained;
its earlier scope/preconditions remain documented in completion/README.md.
