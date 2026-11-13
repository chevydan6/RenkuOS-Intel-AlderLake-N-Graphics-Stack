<!-- SPDX-License-Identifier: MIT -->
# Linux i915 DMA compatibility

This is OS glue for the Linux DMA APIs called by original i915 GEM code.
It uses RenkuOS's existing FreeBSD-compatibility `bus_dma.cpp` and `malloc.cpp`
unchanged. No engine, PTE, submission or GPU cache algorithm is implemented here.
The byte-exact i915 GEM imports remain build targets, not functioning drivers:
their complete translation units still fail on Linux/DRM dependencies.

Implemented scope:

- `dma_set_mask_and_coherent`, sleeping coherent allocation/free, and DMA error
  checking. CPU and DMA addresses satisfy Linux's containing-page-order alignment.
  A kernel-only native RAM alias supplies CPU alignment when needed; the original
  wired backing is retained for native DMA mapping and eventual free.
- `dma_map_sg_attrs`/unmap with original Linux scatterlist layout, traversal and
  initialization. Native map callbacks supply DMA addresses. One output segment
  per input entry is valid; no merging is attempted. Partial failures roll back.
- Existing x86 native direct/coherent mappings only. Streaming spans must remain
  wired, physically contiguous within each entry, within the configured mask,
  and use no bounce buffer. i915's SKIP_CPU_SYNC contract does not permit silently
  substituting bounce-buffer copies. GPU domain/cache transitions belong to i915.

Unsupported coherent atomic allocation/attributes fail explicitly. The binding
registry currently holds 16 opaque device identities and rejects mask changes or
release while mappings live. Native semaphores serialize registry access; callers
must serialize init/uninit and device destruction. General Linux pages/folios,
PCI discovery/attachment, IOMMU setup, IRQ-safe streaming allocation and general
SG allocation/iteration are not supplied. `struct page` currently views caller-owned
native wired backing. Its `virt_to_page`/`nth_page` declarations have no fake bodies.
Only unchanged `sg_next` and `sg_init_table` bodies from scatterlist.c are linked.

Prepare an exclusive bundle on the development host:

```sh
python3 experimental/i915/dma/prepare-bundle.py \
  --source /path/to/RenkuOS/Source --output /path/to/new-bundle
```

Transfer/extract `payload.tar.gz`, then run in actual RenkuOS Terminal with the
matching installed SDK:

```sh
sha256sum -c payload.sha256
bash build-native.sh > build-native.log 2>&1
bash run-native-tests.sh
```

The diagnostic only installs its own `i915_linux_dma_test` driver. It touches no
PCI registers. It verifies real coherent allocation, zeroing, alignment, CPU
patterns, mask/lifetime guards, chained SG mapping and partial rollback; then
checks actual diagnostic unload and unchanged graphics-device publication.
Its synthetic device key tests OS API semantics, not actual N100 GPU DMA activity.

`run-host-tests.py --output NEW_DIRECTORY` runs the real Linux API adapter and SG
helpers under ASan/UBSan with a modeled native backend. Two mutations must fail:
reporting success after a map error and losing partial rollback. This complements,
and does not substitute for, native backend tests. See the evidence directories.

The native link hides unused symbols and garbage-collects unrelated networking
functions in the reused native object. There are no replacement success stubs
for their dependencies. New adapters are MIT; linked Linux SG helpers retain
GPL-2.0 terms as recorded in ../PROVENANCE.md.
