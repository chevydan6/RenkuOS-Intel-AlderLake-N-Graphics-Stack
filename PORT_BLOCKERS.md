<!-- SPDX-License-Identifier: MIT -->
# Demonstrated blockers at the continuation checkpoint

- Whole original intel_ring.c now reaches dma_resv -> ww_mutex. After excluding
  PREEMPT_RT (not selected), compiler evidence/033 identifies missing atomic_long_t,
  lock-class metadata, named mutex initialization/state and current task identity.
  dma-fence.h also reaches asm/errno.h. Wound/wait deadlock handling and fences
  are not implemented; ordinary mutexes cannot silently replace these semantics.
- Partial link still leaves i915_request_timeline and renku_ring_wait_for_space
  unresolved. New imports for the compiled native lock adapter are actual
  KernelExport/OS.h semaphore, spinlock and interrupt APIs. They have not run in
  a native kernel; compilation does not establish their runtime correctness.
- No runnable RenkuOS guest/toolchain or hardware connection is installed here.
  Inspection found no QEMU system binary, cross GCC or SSH agent identities.
  This limits runtime validation; compile artifacts are not hardware evidence.

Resolved, evidence retained:
- Shell Git/DNS failures from first turn: clone now works.
- Missing SupportDefs.h: real headers populated; StorageDefs.h search path fixed.
- ENOSPC/min/max macro conflicts with target headers (013): scoped Linux
  namespace now deliberately separated from native status_t (not ABI-converted).
- Host ASan/UBSan link failure (014): runtimes locally extracted; test passes
  with both sanitizers (019), no checks disabled.
- Previous asm/atomic.h/refcount compilation boundary is resolved by the scoped
  atomic adapter and original refcount code; see 029. Signed-overflow sanitizer
  failure (027) is resolved by Linux's required -fno-strict-overflow build flag,
  preserving all saturation cases and enabled sanitizers (028).
