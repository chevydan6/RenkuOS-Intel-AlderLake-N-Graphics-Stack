# Vulkan buffer-to-image upload, N100 checkpoint 102

The experimental Vulkan driver now records a whole-image
`vkCmdCopyBufferToImage` into its linear BGRA8 render target. It accepts one
tightly packed 32×32 or 64×64 color region in `VK_IMAGE_LAYOUT_GENERAL`.
The image must have transfer-destination usage. The buffer, image, region,
and disjoint memory ranges are validated before submission. An ordered
transfer event calls the existing native owner COPY request; no kernel
binary or Linux graphics interface was added.

The copy record now stores owner-managed memory handles and byte offsets
directly. Buffer-to-buffer, image-to-buffer, and buffer-to-image transfers
share that one replay path. It reports source and destination kinds and the
blocking ioctl time when tracing is enabled.

The N100 probe draws a first triangle, uploads a solid `0xff336699`
background over that image, then runs a second render pass with LOAD and
draws a different triangle. This exercises draw → upload → preserving draw
in one command buffer. At 32×32, the owner reported 4096 copied bytes and
555 µs for the copy; at 64×64, 16384 bytes and 583 µs. These are individual
blocking ioctl samples, not a performance benchmark.

`verify-upload.py` compares each complete raw capture against an independent
checkpoint-082 capture of the second triangle, replacing only its known
poison background with the uploaded color. Both comparisons report zero
mismatches:

| Size | Pixels | SHA-256 of final image |
|---|---:|---|
| 32×32 | 1024 | `83eeac9914e2e3c718d9d8e541360ed7e7cf4befbe5182a25d8eee5c498fd913` |
| 64×64 | 4096 | `ae0868283549a198aea87baeaf9aaf25a562b65d15ef8eec6a82663f7b741156` |

The refactored buffer-to-buffer vertex copy and image-to-buffer readback
each passed one focused physical N100 regression run. The Haiku VM compiled
the Vulkan library with `-Werror`; its compiler, API, fence, semaphore, and
command-reset probes passed. The test probe needed two harness corrections:
its mode-count guard initially rejected a two-pass workload, and its format
query expected the old transfer-feature mask. Both failures happened before
submission. The final probe was rebuilt with `-Werror` and ran on hardware.

The candidate library and probe are isolated at
`/boot/home/native-imageupload-102` on the N100. Library SHA-256 is
`86d2e2e0f0268d5803a7fa8f2a507e6fecfa66abac7ce4435b53ddb4a970d57f`.
The installed owner and bridge remain at checkpoint 100. The library has no
libdrm dependency.

Selected runtime source is approximately **14,831,602 bytes**, a 1,701-byte
increase over checkpoint 101's selected inventory. This is a partial source
estimate, not a complete kernel plus userspace model-token measurement. The
3–5 MB target and broad Vulkan support remain open. Subrectangles, row
pitches, other formats, explicit image barriers, and Vulkan conformance are
not covered by this capability. The full 262-draw hardware suite was not
repeated for this focused increment.
