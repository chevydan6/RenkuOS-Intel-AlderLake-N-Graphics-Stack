<!-- SPDX-License-Identifier: MIT -->
# Native pinned-page dirty/accessed bridge — 2026-11-14

Source candidate: [a3d17e11](https://github.com/chevydan6/haiku-intel-gpu/commit/a3d17e11de83ec034c669bac88deb9651a658de1),
tree `e0990e5771112758adddc2a6cd86900aefbbe941`. Native GCC/G++13.3 on the physical
N100, RenkuOS hrev60072+518, passed four **57,807-check** completion/memory runs
and four **650-check** VMA runs: **233,828 checks, zero functional failures**.
Version11 retains a128-byte report. All7,118 source hashes before/after, all13
production hashes, diagnostic unloads and app_server PID127 are preserved.
See [summary.json](summary.json), [native-build.log](native-build.log),
[native-run.log](native-run.log) and lifecycle/binary hash records.

`renku_vm_page_state_update` reaches actual native vm_page modified/accessed fields
under the real cache lock/reference. Callers hold a writable native pin and use
sleepable context throughout. The operation ORs requested flags and returns prior
software flags; zero queries. Invalid masks, null output, unaligned addresses and
disabled interrupts fail without changing output. Physical identity alone does
not convey ownership. This is not Linux atomic page setters, dirty-page/writeback
tracking, Linux LRU behavior or hardware PTE flag harvesting. There is no production
clear operation or successful placeholder for those services.

After correcting a missing native include in the test helper, the RED build
reached the expected undefined bridge symbol. The independent native oracle
clears mapping/software flags with vm_clear_map_flags and reads actual SDK fields
on newly allocated private diagnostic backing. Tests start clear, check separate,
combined and repeated updates, previous values and rejected requests while flags
remain clear. Two gated, joined native workers perform512 updates per run. A
writable pin protects every clear, lookup, update and inspection.

Both omitted-write controls fail functional assertions: [no-dirty](controls/no-dirty/runtime.log)
has7 failures; [no-accessed](controls/no-accessed/runtime.log) has4. Both exit1 and
unload cleanly. These expected negative results are separate from the positive
suite. The restored final source passes again. Host regressions pass57,729 checks
under ASan/UBSan plus existing controls. All100 include cases,18 scope rejections,
four unsupported-operation link controls, target-header C compilation and118
byte-exact Linux imports retain their intended results. [Independent review](review.md)
found no material defects; its two minor test suggestions were incorporated.

Reproduce with a fresh completion/prepare-bundle.py bundle from the pinned Linux
and RenkuOS sources. Preserve the trusted baseline-components.sha256, copy
payload.sha256 to final-payload.sha256, acquire the physical lease, display live
logs through NanoKVM, and use [visible-run.sh](visible-run.sh), adjusting its fresh
directory. For [control-run.sh](control-run.sh), copy the published
controls/<name>/source.cpp files into mutations/<name>.cpp in that bundle. The
control runner restores source; rebuild it before final tests. Do not overwrite
prior evidence directories. No production graphics driver is replaced.

Native time incorrectly reports October1; session UTC is November14. Content
hashes identify the run. Raw evidence/binaries remain in
/boot/home/i915-native-state-001 and locally in
/home/dan/projects/i915-native-state-evidence-001; summary.json records the archive
digest. Credentials, raw syslogs/process captures and binaries are not published.
The physical lease was released after cleanup; authorized SSH remains available.

All four complete original sources still fail compilation; see [whole-build](whole-build).
Ring/GTT need process-mm/task semantics, while physical/shmem GEM reach Linux VM
layouts through swap.h. Next connect native state to managed Linux pages with
correct atomic ownership, propagation-before-unpin and newly-dirty semantics.
GFP/reclaim, writeback/truncate, user mappings, full-source closure and actual GPU
submission/retirement remain required work.
