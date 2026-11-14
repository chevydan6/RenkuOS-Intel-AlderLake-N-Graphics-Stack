# Native page-state implementation plan

> **For agentic workers:** Use superpowers:executing-plans inline, with a fresh
> whole-change reviewer after verification. Continue under existing authorization.

**Goal:** Reach actual native dirty/accessed page flags for the original i915 port.

**Architecture:** A C ABI isolates real C++ VM layouts from Linux compatibility
headers. A writable native pin protects physical identity, and the actual native
cache lock serializes updates. Linux atomic setters remain unresolved.

**Tech Stack:** Native GCC/G++13.3, real installed RenkuOS SDK, C11/C++17.

**Spec:** docs/superpowers/specs/2026-11-14-native-page-state-design.md

## Global constraints

- Original imported files remain byte-exact; new glue is MIT.
- No production driver replacement or unowned physical-page access.
- Native pin stays held across every lookup/update/test inspection.
- No successful no-op, fake VM layouts or claims of Linux writeback support.

## Review focus

- Invalid inputs must not change output or native flags.
- Concurrent updates preserve both bits and release every cache reference.
- Test flags must start clear so omitted updates are observable.
- Native cache locks require sleepable context and held pins.
- Diagnostic removal must actually unload the module.

### Task 1: Native state bridge and independent oracle

Files: include/native_page_bridge.h, src/native_page_bridge.cpp,
completion/native_page_state_test.cpp, completion/native_page_state_cases.h,
completion/cases.h, completion/build-native.sh, completion/{test.h,native_client.c,host_test.c}.

Interface: int32_t renku_vm_page_state_update(uint64_t physical,
uint32_t requested, uint32_t *previous); flags DIRTY=1, ACCESSED=2; native status.

- [ ] Add native tests that independently clear/read real vm_page flags, then
  require dirty/accessed transitions, old-state values, unchanged invalid outputs,
  disabled-interrupt rejection and two joined workers with 256 updates each.
- [ ] Build natively and observe missing state-update symbol before implementation.
- [ ] Implement bridge using actual vm_page/VMCache headers and cache lock.
- [ ] Build natively, run completion suite and mutation controls omitting dirty
  or accessed writes. Require independent oracle failures and clean unload.

### Task 2: Regression evidence and checkpoint

- [ ] Run host completion regressions, target-header compilation and import hashes.
- [ ] Record native source/binary hashes, diagnostics, unload and baseline checks.
- [ ] Obtain independent whole-change review, resolve material findings, update
  handoff/progress/blockers with exact limits, and publish a signed checkpoint.
- [ ] Keep full i915/GPU objective active and name the next integration dependency.
