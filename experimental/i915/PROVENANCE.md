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
