<!-- SPDX-License-Identifier: MIT -->
upstream/gt/intel_ring.c and intel_ring.h are byte-exact Linux v6.6 files.
Copyright © 2019 Intel Corporation; SPDX MIT, retained in original files.

Ported-from: Linux/v6.6/drivers/gpu/drm/i915/gt/intel_ring.c
Ported-from-license: MIT

Ported-from: Linux/v6.6/drivers/gpu/drm/i915/gt/intel_ring.h
Ported-from-license: MIT

ring_port.h __intel_ring_space and intel_ring_wrap retain the corresponding
Linux helper algorithms. Other adapters and extraction/build tooling are MIT.
No complete Linux kernel or FreeBSD subsystem is imported.

## Complete DRM allocator imports

All source paths are relative to Linux v6.6, commit
ffc253263a1375a65fa6c9f62a893e9767fbebfa. See upstream-manifest.json for
Git blob IDs and SHA256 hashes. Imported source/headers remain byte-exact.

Ported-from: Linux/v6.6/drivers/gpu/drm/drm_mm.c
Ported-from-license: MIT

Ported-from: Linux/v6.6/include/drm/drm_mm.h
Ported-from-license: MIT

Ported-from: Linux/v6.6/lib/rbtree.c
Ported-from-license: GPL-2.0-or-later

Ported-from: Linux/v6.6/include/linux/rbtree.h
Ported-from-license: GPL-2.0-or-later

Ported-from: Linux/v6.6/include/linux/rbtree_types.h
Ported-from-license: GPL-2.0-or-later

Ported-from: Linux/v6.6/include/linux/rbtree_augmented.h
Ported-from-license: GPL-2.0-or-later

Ported-from: Linux/v6.6/include/linux/interval_tree_generic.h
Ported-from-license: GPL-2.0-or-later

Ported-from: Linux/v6.6/include/linux/list.h
Ported-from-license: GPL-2.0-only

The original license notices and author attribution remain in each import.
upstream/GPL-2.0.txt retains Linux's GPL-2.0 license text. GPL helper code is
confined to this experiment; the combined experimental allocator/object is
subject to GPL-2.0, while new adapter/tooling files remain MIT. This does not
change the license of the existing display implementation or root repository.

## Reference-count imports

Ported-from: Linux/v6.6/lib/refcount.c
Ported-from-license: GPL-2.0-only

Ported-from: Linux/v6.6/include/linux/refcount.h
Ported-from-license: GPL-2.0-only

Ported-from: Linux/v6.6/include/linux/kref.h
Ported-from-license: GPL-2.0-only

## Wound/wait and errno imports

Ported-from: Linux/v6.6/include/linux/ww_mutex.h
Ported-from-license: GPL-2.0-only

Ported-from: Linux/v6.6/include/linux/err.h
Ported-from-license: GPL-2.0-only

Ported-from: Linux/v6.6/include/linux/errno.h
Ported-from-license: GPL-2.0-only

Ported-from: Linux/v6.6/include/uapi/asm-generic/errno-base.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/uapi/asm-generic/errno.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/uapi/linux/errno.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note (upstream COPYING; wrapper has no file-specific SPDX)

Ported-from: Linux/v6.6/include/uapi/linux/wait.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

The original Linux syscall exception text is retained in
upstream/licenses/Linux-syscall-note. Native task/time/lock adapters are new MIT
code. FreeBSD ww_mutex/linux_lock code was consulted, not imported.

upstream/COPYING retains Linux's root license declaration, including the
syscall exception applicable to the one-line UAPI errno include wrapper.

## Batch-start execution slice

Ported-from: Linux/v6.6/drivers/gpu/drm/i915/gt/gen8_engine_cs.c
Ported-from-license: MIT

Ported-from: Linux/v6.6/drivers/gpu/drm/i915/gt/intel_gpu_commands.h
Ported-from-license: MIT

I915_DISPATCH_SECURE in emitter_port.h retains BIT(0) from Linux v6.6
intel_engine_types.h (MIT, Intel Corporation 2019). Original emitter bodies and
intel_ring_advance are extracted unchanged; native object/query interfaces and
CPU integration tests are new MIT code. Tests are not GPU execution evidence.

Ported-from: Linux/v6.6/include/linux/scatterlist.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/dma-direction.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/drivers/gpu/drm/i915/i915_gem_gtt.c
Ported-from-license: MIT

Ported-from: Linux/v6.6/drivers/gpu/drm/i915/gem/i915_gem_phys.c
Ported-from-license: MIT

Ported-from: Linux/v6.6/lib/scatterlist.c
Ported-from-license: GPL-2.0-or-later

The byte-exact scatterlist header is used directly. Initial DMA compilation
links unchanged sg_next/sg_init_table bodies; other scatterlist.c operations
remain outside that link until their kernel dependencies are ported.

The native DMA bundle exports bus_dma.cpp, malloc.cpp and their surrounding
headers unchanged from RenkuOS revision 4799bb5a188c4bb6bbef967287640cc21a061a0d.
The original license notices remain in that exported tree; it is not vendored here.

Ported-from: RenkuOS/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/libs/compat/freebsd_network/bus_dma.cpp
Ported-from-license: MIT

Ported-from: RenkuOS/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/libs/compat/freebsd_network/malloc.cpp
Ported-from-license: MIT

## Compiler helpers and DRM VMA offset manager

Ported-from: Linux/v6.6/include/linux/compiler_types.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/log2.h
Ported-from-license: GPL-2.0-or-later

Ported-from: Linux/v6.6/drivers/gpu/drm/drm_vma_manager.c
Ported-from-license: GPL-2.0 OR MIT

Ported-from: Linux/v6.6/include/drm/drm_vma_manager.h
Ported-from-license: MIT

These four imports are byte-exact and manifest-verified. The VMA manager builds
as a complete translation unit with native reader/writer and heap adapters;
it does not provide actual user address-space mapping or a GEM object manager.
New wrappers use GCC bit-scan primitives and RenkuOS's existing rw_spinlock,
IRQ, wired-area and deferred-free services. No native lock algorithm is copied.

## Original arithmetic, configuration and completion/wait code

Ported-from: Linux/v6.6/include/asm-generic/div64.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/lockdep_types.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/kconfig.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/uapi/linux/types.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/uapi/linux/posix_types.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/uapi/asm-generic/int-ll64.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/uapi/asm-generic/posix_types.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/arch/x86/include/uapi/asm/posix_types_64.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/linux/completion.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/swait.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/wait.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/lockdep.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/kernel/sched/completion.c
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/kernel/sched/swait.c
Ported-from-license: GPL-2.0

The complete original kernel/sched/completion.c and swait.c compile with
completion_port.h supplying the surrounding compilation environment normally
provided by Linux sched/build_utility.c. Their algorithms and public headers
remain byte-exact. New event/deadline/task-scope glue is MIT and uses native
semaphores, IRQ locks and clocks; it is not a general Linux scheduler.

## Canonical compiler, arithmetic and byte-order helpers

Ported-from: Linux/v6.6/include/linux/compiler_attributes.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/build_bug.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/const.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/linux/minmax.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/math.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/align.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/vdso/const.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/uapi/linux/const.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/uapi/linux/kernel.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/uapi/linux/sysinfo.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/linux/byteorder/little_endian.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/byteorder/generic.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/uapi/linux/byteorder/little_endian.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/linux/swab.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/uapi/linux/swab.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/arch/x86/include/uapi/asm/swab.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/linux/panic.h
Ported-from-license: GPL-2.0

linux/const.h has no file-level notice; its companion license attribution uses
the pinned Linux root COPYING declaration, retained unchanged in upstream/COPYING.
Original helper files remain byte-exact. Native C library declarations are parsed
before selecting Linux macro spellings. New IRQ-scope/preemption adapters are MIT
and reuse existing native interrupt and lock services. The lower_32_bits spelling
in kpi_base.h now matches the helper in Linux/v6.6/include/linux/kernel.h.

## Original bit operations, bitmap search and ioctl encoding

Ported-from: Linux/v6.6/include/linux/bitops.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/bits.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/typecheck.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/vdso/bits.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/asm-generic/bitsperlong.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/find.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/bitmap.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/lib/find_bit.c
Ported-from-license: GPL-2.0-or-later

Ported-from: Linux/v6.6/lib/hweight.c
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/uapi/asm-generic/ioctl.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/asm-generic/bitops/generic-non-atomic.h
Ported-from-license: GPL-2.0-only

Ported-from: Linux/v6.6/include/asm-generic/bitops/non-atomic.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/asm-generic/bitops/non-instrumented-non-atomic.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/asm-generic/bitops/__ffs.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/asm-generic/bitops/__fls.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/asm-generic/bitops/ffz.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/asm-generic/bitops/fls.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/asm-generic/bitops/fls64.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/asm-generic/bitops/hweight.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/asm-generic/bitops/arch_hweight.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/asm-generic/bitops/const_hweight.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/uapi/asm-generic/bitsperlong.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

## Original CPU-mask, cache-alignment and limits definitions

Ported-from: Linux/v6.6/include/linux/cpumask.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/threads.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/numa.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/cache.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/arch/x86/include/asm/cache.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/linkage.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/stringify.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/linux/limits.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/vdso/limits.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/uapi/linux/limits.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/asm-generic/param.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/uapi/asm-generic/param.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

linux/stringify.h has no file-level license notice; attribution follows
the pinned root COPYING declaration, retained unchanged in upstream/COPYING.

## Original raw/regular/rw lock structures and bit locking

Ported-from: Linux/v6.6/include/linux/spinlock_types_raw.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

Ported-from: Linux/v6.6/include/linux/spinlock_types.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/rwlock_types.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/bit_spinlock.h
Ported-from-license: GPL-2.0

spinlock_types_raw.h has no file-level notice; its attribution follows the
pinned root COPYING declaration. spinlock_types.h and rwlock_types.h retain
their original GPL notices. No SPDX text was inserted into byte-exact imports.

Ported-from: Linux/v6.6/lib/bitmap.c
Ported-from-license: GPL-2.0-only

Only the complete original __bitmap_equal function is currently extracted for
linking. The containing source is a byte-exact import; allocation, parsing and
other external bitmap algorithms remain unported, not successful stubs.

Ported-from: Linux/v6.6/include/linux/kern_levels.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/kdev_t.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/uapi/linux/kdev_t.h
Ported-from-license: GPL-2.0 WITH Linux-syscall-note

`linux/lockref.h` remains byte-exact.

Ported-from: Linux v6.6 ffc253263a1375a65fa6c9f62a893e9767fbebfa include/linux/lockref.h
Ported-from-license: GPL-2.0

`lib/lockref.c` remains byte-exact.

Ported-from: Linux v6.6 ffc253263a1375a65fa6c9f62a893e9767fbebfa lib/lockref.c
Ported-from-license: GPL-2.0

`linux/gfp_types.h` remains byte-exact.

Ported-from: Linux v6.6 ffc253263a1375a65fa6c9f62a893e9767fbebfa include/linux/gfp_types.h
Ported-from-license: GPL-2.0

`linux/gfp.h` remains byte-exact.

Ported-from: Linux v6.6 ffc253263a1375a65fa6c9f62a893e9767fbebfa include/linux/gfp.h
Ported-from-license: GPL-2.0

`linux/seqlock.h` remains byte-exact.

Ported-from: Linux v6.6 ffc253263a1375a65fa6c9f62a893e9767fbebfa include/linux/seqlock.h
Ported-from-license: GPL-2.0

`linux/kcsan-checks.h` remains byte-exact.

Ported-from: Linux v6.6 ffc253263a1375a65fa6c9f62a893e9767fbebfa include/linux/kcsan-checks.h
Ported-from-license: GPL-2.0

`linux/highmem-internal.h` remains byte-exact; direct-map atomic helpers are extracted unchanged.

Ported-from: Linux v6.6 ffc253263a1375a65fa6c9f62a893e9767fbebfa include/linux/highmem-internal.h
Ported-from-license: GPL-2.0

`linux/mm.h` remains byte-exact; only manifested pure helpers are selected from mm/page-flags.

Ported-from: Linux v6.6 ffc253263a1375a65fa6c9f62a893e9767fbebfa include/linux/mm.h
Ported-from-license: GPL-2.0

`linux/page-flags.h` remains byte-exact; only manifested pure helpers are selected from mm/page-flags.

Ported-from: Linux v6.6 ffc253263a1375a65fa6c9f62a893e9767fbebfa include/linux/page-flags.h
Ported-from-license: GPL-2.0

`linux/pagevec.h` remains byte-exact and supplies the complete folio-batch header.

Ported-from: Linux v6.6 ffc253263a1375a65fa6c9f62a893e9767fbebfa include/linux/pagevec.h
Ported-from-license: GPL-2.0

Complete `gem/i915_gem_shmem.c` is preserved unchanged for actual whole-source porting and compiler validation.

Ported-from: Linux v6.6 ffc253263a1375a65fa6c9f62a893e9767fbebfa drivers/gpu/drm/i915/gem/i915_gem_shmem.c
Ported-from-license: MIT

## Timer and ratelimit interface imports

Ported-from: Linux/v6.6/include/linux/timer.h
Ported-from-license: GPL-2.0

Ported-from: Linux/v6.6/include/linux/ratelimit.h
Ported-from-license: GPL-2.0

Complete headers remain byte-exact, with the original notices. The wrapper
namespaces Linux timer entry points away from native/POSIX APIs. Timer
delivery and cancellation remain unresolved pending their actual native backend.
