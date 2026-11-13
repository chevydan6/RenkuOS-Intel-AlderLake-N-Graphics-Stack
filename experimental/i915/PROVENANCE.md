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
