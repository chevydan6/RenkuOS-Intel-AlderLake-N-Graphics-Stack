<!-- SPDX-License-Identifier: MIT -->
# Native Linux DMA API checkpoint

Intel N100 host, RenkuOS hrev60072+518, installed SDK and GCC13.3. Source checking,
native compilation and tests ran visibly in fullscreen Terminal through NanoKVM.
SSH transferred payloads and collected these files.

The reproducible `linux-dma-002` bundle passes four runs of 45,116 checks:
180,464 checks, zero failures, 16 coherent allocations/frees and 16 streaming
entry maps/unmaps. Actual module unload returns zero; the diagnostic file/link
are removed and graphics device lists match. These are Linux API/native bus_dma
tests using a synthetic device identity, not a PCI-attached i915 driver or GPU
transfer. The same corrected code also passed an earlier four-run validation
in `linux-dma-001`; only the fresh reproducible bundle is counted here.

Coherent sizes are 4K, 8K, 12K and 64K. Both CPU/DMA order alignment, zeroed bytes,
CPU patterns, live-device mask/release guards and wrong-size frees are checked.
Four real wired native pages feed a chained scatterlist including a byte offset.
Duplicate maps, DMA_NONE and failure after a successful first entry are tested.

Preserved failure: the first alignment alias was passed to native bus_dmamap_load
before its lazy RAM mappings existed, panicking in get_memory_map. The correction
maps original wired backing through bus_dma, populates the aligned kernel-only
CPU alias before return, and retains both identities until free. The failure
archive and screenshot hashes/paths are retained in result.json. The machine
rebooted normally with HDMI; the final test driver has been removed.

build-native.log retains pre-existing warnings from the unchanged native BSD
compatibility sources. New bridge/API/test compilation uses Werror. The exact
source payload and binary hashes accompany the evidence. Raw archives remain
outside Git at paths recorded in result.json.

whole-gtt-001.log and whole-phys-001.log are genuine failed complete-source i915
builds. Importing these byte-exact files and validating their DMA prerequisites
does not make their full dependency closure compile. No GPU command ran.
