# A35 descriptor/context-status software validation

`workstreams/gpu-next/IntelN100ContextStatus.h` and
`tests/n100-context-status-test.cpp`: **2357 cases passed** on host ASan/UBSan
and native GCC13.3 in the locked `renkuos-tests` VM. Input hashes, build command,
logs, exit0 and reference hashes are retained here. Native binary was collected
and its hash independently matched the guest log:
`62ce5619250ce54bf8092424aad43a96b16ad6e1cbdb8e1167a9b171e30a1602`.

Tests cover every supported tag1..2046, golden descriptor, full16KiB GGTT bounds,
identity/revision rejection, unchanged output on rejection, status decode,
activation/complete-to-idle sequence, delayed all-ones entry, wrong class/instance,
foreign/stale tag, counter/generation mismatch, waits/preemption, reversed events,
duplicates, extra events and permanent first-failure retention.

Source-derived design: `research/n100-submission/CONTEXT-STATUS-DESIGN.md`.
Pinned reference downloads: `build/a35-context-research-001/`.
Host build: `build/a35-context-host-001/`.
Native build: `build/a35-context-native-001/`, guest
`/boot/home/test-work/a35-context-native-001/`.

These are offline software results. No GPU registers, context memory or
submission were touched. The helper's SwitchedAway state alone cannot authorize
retirement or freeing memory. The actual CSB reader, lifetime integration, full
context image, engine reinitialization and physical event validation remain.
