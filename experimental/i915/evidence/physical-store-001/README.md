<!-- SPDX-License-Identifier: MIT -->
# Physical folio and persistent-store validation — 2026-11-14

The unchanged version 9 candidate from `experiment/i915-linuxkpi` commit
`614bf8ac1b9c702cae40de2bc75e1c5228162aaf` now builds and runs on the physical
Intel N100 with RenkuOS kernel `hrev60072+518` and native GCC/G++ 13.3.0.
The implementation remains `57d16dbf189374d5f883e0673f52c74bb9717699`.

Four completion/memory runs pass 49,556 checks each; four VMA regressions pass
650 checks each. Total: **200,824 checks, zero functional failures**. The actual
C++ page/store bridge compiled and linked against native kernel exports. Each
memory report is version 9, 112 bytes, with seven page allocations/frees, two
page workers and 512 iterations, one store allocation/free, and two store
workers and 256 iterations. Interruptible and ordinary-signal killable cases
also passed. See [native-run.log](native-run.log) and [summary.json](summary.json).

Both diagnostic modules reported successful initialization and unload, and
their installed files, devfs nodes and driver links were absent afterward.
All 13 recorded non-packaged kernel-driver/accelerant hashes remained unchanged;
`app_server` retained PID 127. Results were displayed live in fullscreen native
Terminal through NanoKVM while SSH ran the build/test procedure.

## Reproduction and provenance

The new workstation could read the private repository through the GitHub
connector but lacked shell Git credentials. Its 361 experimental source files
were retrieved at the exact commit and checked against the repository Git blob
IDs. `verify-upstream.py` verified all 118 Linux originals unchanged. Clean
public Linux and RenkuOS sparse checkouts were pinned to the revisions in the
handoff; the original `completion/prepare-bundle.py` generated a complete fresh
bundle with 7,111 hashed files, including the full compiler reference tree.
All 7,111 file checks passed before preparation, before the build and afterward.
The exact hashes of those retained check logs are in `summary.json`.

The existing store-host runner was used with only its working directory changed
to `/boot/home/i915-store-native-20261114`. Compiler outputs, binary hashes,
module lifecycle records and whole-source failures are retained here. Full raw
evidence and binaries remain in the native directory and locally under
`/home/dan/projects/i915-native-evidence-20261114`; the archive hash is recorded
in `summary.json`. Raw syslog/process captures and binaries are not published.

The laptop's clock reported October 1 despite the November 14 UTC session date.
Archive extraction emitted future-timestamp warnings; content hashes passed.
The native clock was not changed. Use source/artifact hashes and the recorded
session context rather than native wall-clock timestamps to identify this run.

## Remaining driver work

All four complete-original compiler attempts still fail. GTT and ring reach
Linux process-mm/task dependencies in `sched/mm.h`, including undefined
`mm_struct` and Linux atomic interfaces. Physical and shmem GEM reach the
deliberately rejected generated Linux VM bounds and missing `asm/page.h` through
`shmem_fs.h`, `swap.h` and `mmzone.h`. Full compiler output is in
[whole-build](whole-build).

This validates the internal folio/store provider on RenkuOS. It does not supply
Linux file/address_space/shmem integration, reclaim/swap-pressure evidence,
writeback/truncate semantics, user mappings, i915 PCI attachment, coherent GPU
DMA, command submission, retirement, accelerated rendering or Vulkan. Continue
the original i915 port; do not replace missing services with successful no-ops.

## Owner clarification

The owner explicitly clarified that watching development does not restrict SSH.
SSH may run development, builds and tests as well as transfers/collection;
keep progress observable through NanoKVM. The earlier keyboard-only execution
restriction in the handoff was an agent interpretation and is superseded.
