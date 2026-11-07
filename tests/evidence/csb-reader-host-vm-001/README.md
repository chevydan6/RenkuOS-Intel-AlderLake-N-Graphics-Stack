# A36 bounded CSB reader / engine-HWSP validation

135 cases passed on host ASan/UBSan and native GCC13.3 in `renkuos-tests`.
Source: `workstreams/gpu-next/IntelN100CsbReader.h` and
`tests/n100-csb-reader-test.cpp`, using the previously validated A35 event ledger.
Design and source citations: `research/n100-submission/CSB-READER-DESIGN.md`.

Coverage includes exact4096-byte page layout and guards; unchanged output on
invalid input; all required callback failures at every successful-path position;
clock failure/rollback/deadline/overflow; frozen-clock iteration bound; deferred
entries without skipping slots; cursor regression even before entry acceptance;
unexpected/high-bit tail values; post-event tail changes; foreign tags,
preemption, reversed/duplicate events; and retained first-failure/no second run.

Input hashes and native binary hash were independently checked after collection.
Native binary SHA-256:
`c7e9f2160c7ef30ba60d9bef795f035849acfc26c3a59b529707420b2e4507fd`.
Host artifacts: `build/a36-csb-host-001/`.
Native artifacts: `build/a36-csb-native-001/`; guest workspace:
`/boot/home/test-work/a36-csb-native-001/`.

VM build ran through `vm.py run` under its transaction lock after a nonblocking
lock acquisition for transfer. No VM reset/rollback or Qwen workspace change.
Kernel/SDK hashes match the prior baseline. This is software evidence only;
callbacks were injected, with no GPU access or hardware retirement claim.

EventsObserved alone cannot authorize freeing memory. The production owner,
cache visibility, hardware pointer reset, error service and full context image
remain integration work. An unseen full FIFO revolution cannot be detected
from a modulo pointer alone and must be excluded by the isolated execution
ownership protocol.
