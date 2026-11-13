<!-- SPDX-License-Identifier: MIT -->
ASan/UBSan tests of the real DMA API adapter and unchanged Linux SG helpers pass.
The native backend is modeled here. Mapping failures at each position, rollback,
chained traversal, duplicate mapping, invalid directions/attributes and coherent
allocation modes are exercised. Both intentionally broken mutation controls abort
with assertions; see negative-controls.json. Actual native backend results are
recorded separately in ../physical-linux-dma-001.
