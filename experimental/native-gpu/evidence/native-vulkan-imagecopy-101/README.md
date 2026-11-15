# Native Vulkan image-to-buffer readback, N100 checkpoint 101

The experimental Vulkan driver now records a whole-image
`vkCmdCopyImageToBuffer` after a render pass. The admitted image is its
existing linear BGRA8 32×32 or 64×64 render target, created with transfer
source usage and left in `VK_IMAGE_LAYOUT_GENERAL`. The destination is a
bound transfer-destination buffer with a tightly packed, single-layer color
region. The driver validates the region and appends it to the existing
ordered copy/draw event stream. Submission uses the native owner-side COPY
request from checkpoint 100; the installed kernel owner and bridge did not
change.

The updated probe renders a triangle, records image-to-buffer copy in the
same command buffer, waits for the submission fence, invalidates the mapped
destination buffer, and hashes those buffer bytes. The older Vulkan library
failed to link the probe because it lacked `vkCmdCopyImageToBuffer`.

On the physical N100, both target sizes passed. Raw destination-buffer
captures match independent checkpoint-082 single-triangle captures byte for
byte:

| Size | FNV-1a pixel hash | SHA-256 of raw BGRA bytes |
|---|---|---|
| 32×32 | `28616d2d` | `d078ce6ad13cc5e13e85f660b0ea53a2963a47ad619297d7796f82e1f82627eb` |
| 64×64 | `8aaee2f5` | `c3cd98385b4fc4dc40f92bec112697c8ecaabe670c029db25c6f18217591882d` |

The trace shows RCS completion at stage 12 followed by native copy success.
The 32×32 copy transferred 4096 bytes in 493 µs; the 64×64 copy transferred
16384 bytes in 505 µs in these single runs. The prior buffer-to-buffer
`copyvertex` workload also passed with the refactored command recording.
The Haiku VM compiled the library and probe with `-Werror` and passed the
compiler, API, fence, semaphore, and command-reset probes. No full hardware
regression suite was repeated.

Candidate binaries are isolated at `/boot/home/native-imagecopy-101` on the
N100. `libvulkan.so` SHA-256 is
`112cafdf76c55b79fcdcdd20fc68758dbdb3bf08b672bf285e7d7f994530e136`;
the installed owner/bridge hashes remain those of checkpoint 100. Runtime
dependencies contain the pinned Intel compiler and Haiku libraries, without
libdrm.

The selected runtime source estimate is **14,829,901 bytes**, 2,422 more
than checkpoint 100. It remains a partial source inventory rather than a
complete kernel plus userspace model-token measurement. The 3–5 MB source
target and broad Vulkan compatibility remain open. This path currently
supports one full tightly packed image region; subrectangles, row pitches,
layout transitions, and general image formats are not yet implemented.
