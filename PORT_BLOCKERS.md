<!-- SPDX-License-Identifier: MIT -->
# Demonstrated blockers at the continuation checkpoint

- Whole original intel_ring.c now parses ww_mutex and DMA-fence declarations and
  fails in DRM VMA manager interfaces: rwlock_t, read_lock/read_unlock/rwlock_init,
  then __user/raw_spinlock completion and IDR/radix-tree header compatibility (062).
  Earlier memset/__u64/VM-area declarations from 046 are now supplied. The native page constants
  are checked, but VM mapping/unmapping/DMA allocation is not implemented.
- Original ww inline code emits an ELF object, but ww_mutex_lock and
  ww_mutex_lock_interruptible remain undefined in the actual partial link (047).
  Deadlock avoidance, waiter wakeup/cancellation, RCU lifetime and real fence
  waiting/signaling remain unresolved, not silently simulated.
- Request timeline and ring space waiting/retirement remain undefined. New native
  task/time adapters import real find_thread and system_time_nsecs. Registered
  task scopes require caller lifetime discipline; they are not general scheduler
  objects or task references. Scoped task/time/lock paths now pass actual guest
  kernel worker tests; GPU retirement remains unimplemented.
- Native-compiled execution object still imports i915_request_timeline and
  renku_ring_wait_for_space (native-vm-006/execution-imports.txt). No GPU backing
  mappings, real context submission or completion connects that object.

Resolved, evidence retained:
- Missing VM/toolchain: actual RenkuOS build00081 runs under KVM with matching
  SDK. Native driver passes 16 × 62 checks; UNINIT status=0 confirms teardown.
- Physical target access: restored using the user-supplied NanoKVM connection.
  Actual N100 hrev60072+518 compilation and kernel-service suite pass another
  16 × 62 checks with matching GCC13.3/SDK. No GPU-execution result is implied.
- Kernel-worker join panic (066): exact guest source establishes no cached
  death entry for kernel threads. Completed, absent-ID late joins are handled
  explicitly; live/unknown-error cases remain fatal. Final native tests pass.
- Reinstall devfs ENOENT (067): asynchronous publication now has a bounded wait.
  Native allocation/semaphore cleanup and invalid-resource tests pass.
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
- Previous atomic_long, mutex named initialization/state and current-identity
  compile failures from 033 are resolved for scoped execution, tested in 047/048.
  errno/error-pointer namespace failures are resolved; all original pointer codes
  and preservation of native B_* status aliases passed the target-header probe.
- The original batch-start emitter and ring slice link and execute CPU packet
  construction (056/057), but still use private request/ring layouts. They lack
  real GT/context/mapping/submission/completion integration and cannot be passed
  full Linux i915 request objects or counted as GPU command execution.
