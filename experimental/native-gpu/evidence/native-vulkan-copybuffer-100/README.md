# Native Vulkan buffer copy, N100 checkpoint 100

This increment adds `vkCmdCopyBuffer` to the experimental Vulkan driver.
Command buffers retain copy and draw events in recording order, including a
copy followed by a draw in one submission. Buffer creation now accepts
combinations of vertex, index, uniform, transfer-source, and
transfer-destination usage bits. Copy regions are checked for size, bounds,
alignment, and overlapping aliases before submission.

The native resource ioctl gained a bounded COPY action using its existing
4136-byte request layout: `gpu` carries the source handle, `size` the source
offset, `handle` the destination, `offset` the destination offset, and `bytes`
the length. The bridge calls the owner's new `native_resource/v2` module
entry. The owner resolves both live handles in the same session, rejects
overlap, waits for GPU visibility, and copies through its prebound CPU
mappings. No Linux DRM/GEM interface is involved. This is an owner-side CPU
copy of GPU buffers, not a BCS DMA copy.

The new `copyvertex` probe loads one triangle into a staging buffer and a
different triangle into the destination buffer. It records a 48-byte copy,
then renders from the destination. On the physical N100, the copy completed
and RCS rendered the expected image at both 32×32 and 64×64. The raw BGRA
captures are byte-identical to independent single-triangle captures from
checkpoint 082:

| Image | SHA-256 |
|---|---|
| 32×32 | `d078ce6ad13cc5e13e85f660b0ea53a2963a47ad619297d7796f82e1f82627eb` |
| 64×64 | `c3cd98385b4fc4dc40f92bec112697c8ecaabe670c029db25c6f18217591882d` |

With the final installed owner and bridge, a traced 32×32 run reported
`vk_copy_buffer` status 0, source BO 129, destination BO 130, 48 bytes, and
563 µs for the blocking copy ioctl. The subsequent RCS draw used destination
BO 130, completed at stage 12, and matched all 777 pixels checked by the
owner. One previous indexed mesh and one two-triangle native batch also
passed. The old Vulkan library failed to link the updated probe because it
did not provide `vkCmdCopyBuffer`.

The Haiku VM built the owner and bridge with `-Werror`; their existing CPU
smokes passed. The compiler-backed Vulkan library built with `-Werror`, and
the compiler, API, fence, semaphore, and command-reset probes passed in the
VM. The N100 booted the candidate owner and bridge. Installed SHA-256 hashes:

- Owner: `0a16e8bf6dbe92238ffd00d1ad50cefe057c3cbcccb443f3f7be864c4f96901a`
- Bridge: `584458e7c1dadfcedbeed910d7ef1a4b2483372e6bdbaaf14ba789a214b4aca7`
- Candidate Vulkan library: `14497e4b8d8451cb404857f8d26ed278b1042f059419a504d05e1840cbfd608d`

The previous owner and bridge are preserved at
`/boot/home/native-copybuffer-100/rollback-*` on the N100. The new Vulkan
library and probe are in that same isolated directory. The library's runtime
dependencies contain the pinned Intel compiler and Haiku libraries, with no
libdrm dependency.

Selected runtime source is approximately **14,827,479 bytes**, computed
from checkpoint 099's selected closure and the 6,947-byte change in five
runtime files. This is a partial source inventory, not a full kernel plus
userspace model-token measurement. The 3–5 MB source target remains unmet.
Only the 48-byte copy-then-draw path was exercised on hardware here; larger
and cross-page copies still need application-driven validation. The full
262-draw hardware suite was not repeated for this focused increment.
