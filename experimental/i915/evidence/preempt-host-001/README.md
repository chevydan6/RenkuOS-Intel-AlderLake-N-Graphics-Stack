<!-- SPDX-License-Identifier: MIT -->
# Host checks for the native preemption continuation

These are host results, distinct from physical-preempt-001. completion runs the
original completion/swait code under ASan/UBSan with pthread/POSIX services and
modeled IRQ state (122 checks, 24 waiters). Lost completion-count mutation aborts;
original BUILD_BUG_ON(1) and mixed-type min controls must fail compilation, while
the positive helper case compiles.

irq-model compiles the actual native IRQ/preemption/spin/rw adapter sources with
real target headers and ASan/UBSan, then substitutes modeled native services.
Serialized two-CPU state changes are checked; there is no concurrency/hardware
claim. Its premature-IRQ-enable mutation exits 90 as required. vma and dma retain
sanitizer API tests and rejected mutation controls. Exact commands are retained
in build logs where emitted; scripts are under completion/, vma/, dma/ and tests/.
Component allocator/refcount/ww/emitter and target-header probe logs accompany
the physical evidence. Raw binaries and intermediary failures remain outside Git
under /home/dan/i915-vm/preempt-port-001/.
