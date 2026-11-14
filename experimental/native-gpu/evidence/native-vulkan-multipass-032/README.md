# Two render passes in one native Vulkan command buffer

The Vulkan command buffer now stores a growable sequence of draw records.
Each record captures its framebuffer, graphics pipeline, and vertex buffer at
`vkCmdDraw`. A later bind does not change an earlier draw. A command buffer may
record multiple render-pass instances, each with one bounded triangle draw.
`vkQueueSubmit` validates every record before submitting any, replays the
records in order through the native owner, writes each completed target to its
bound image BO, and signals the fence after the last successful draw. Trace
records include `draw_index` and `draw_count`.

The VM command probe failed with exit 7 against the previous library. With
the new library it recorded five render passes, crossed the initial
four-record allocation, checked snapshots of two distinct pipeline and vertex
bindings, and verified both retained and release-resources reset behavior.
The full VM compiler/API/fence/reset build passed (`vm-build.log`). The
compiler-free library variant also compiled with `-Wall -Wextra -Werror`
(SHA-256 `2e04b0dbbb89e8766de122e737606671a6ae1e08202dbbdfff678b000f1164f2`).

The physical probe was staged at `/boot/home/native-vulkan-multipass-032/`.
The previous userspace library rejected the new two-pass recording before
submission (probe code 25, `old-library-rejection.log`). With the candidate
library, one command buffer produced two N100 owner submissions with distinct
shader BOs. Their exact image hashes were `28616d2d` and `1629ed2d`; the
mapped image held the second result. The two-command-buffer dual-pipeline
path, visible vertex rotation, depth vertex program, and version-7 batch also
passed without reported GPU fault (`physical.log`). Binary hashes are in
`physical-hashes.log`. No owner or bridge binary changed, and the physical
test lock was verified free afterward.

The current owner replaces the full 32×32 target for each draw. This
checkpoint therefore supports separate render passes whose attachments have
`LOAD_OP_DONT_CARE`. Multiple draws within one render pass, preserving the
prior attachment, and general Vulkan load/store behavior remain future work.
