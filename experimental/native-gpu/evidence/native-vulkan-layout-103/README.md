# Vulkan image layout tracking, N100 checkpoint 103

The experimental Vulkan driver now records layout transitions for its
single-color-attachment render passes and a bounded `vkCmdPipelineBarrier`
image barrier. Queue submission simulates the ordered image layouts across
all submitted command buffers before starting work. It refuses a copy or
draw whose required layout does not match. Successful execution updates the
image's layout state and emits concise `vk_image_layout` trace events.
The layout model follows Vulkan's [render-pass transitions](https://docs.vulkan.org/spec/latest/chapters/renderpass.html)
and [image memory barriers](https://docs.vulkan.org/refpages/latest/refpages/source/VkImageMemoryBarrier.html)
for this limited image shape.

The N100 probe creates an undefined linear BGRA8 image, records an
undefined→general image barrier, uploads a solid `0xff336699` background
through `vkCmdCopyBufferToImage`, then begins a LOAD render pass and draws
one triangle. The owner rendered the 32×32 and 64×64 images on RCS and
reported 777/777 and 3476/3476 checked pixels respectively. The native
copy ioctl transferred 4096 and 16384 bytes. `verify-layout.py` compares
every output pixel against independent checkpoint-082 first-triangle
captures with only their poison background replaced. Both comparisons have
zero mismatches:

| Size | Pixels | SHA-256 of final BGRA image |
|---|---:|---|
| 32×32 | 1024 | `f2225634ab79767e9610e810f9e880d794ae4016e563850b61a1b5bc5b08621c` |
| 64×64 | 4096 | `5d9b3fa7ae8ab0954954955875eb424b85939a7ff6fd85a85fce3c84f5874fe7` |

The negative `barrierbad` probe supplied `oldLayout=GENERAL` for an image
whose actual layout was still UNDEFINED. Queue preflight returned
`VK_ERROR_INITIALIZATION_FAILED` and left the fence unsignaled; no RCS draw
was submitted. Focused hardware regressions passed for the previous
two-pass image upload, image-to-buffer readback, two-triangle native batch,
and indexed mesh. The full 262-draw suite was not repeated.

The Haiku VM built the candidate library with `-Werror` and passed the
compiler, API, fence, semaphore, and command-reset probes. The old library
failed to link the updated probe because it lacked `vkCmdPipelineBarrier`.
The candidate library and final probe are isolated at
`/boot/home/native-layout-103` on the N100. Their SHA-256 hashes are
`f79a03e54d571609634c0d070753652299e1d26e6dcf15f7002e399d2da0d688`
and `e36752750ca164f8e99814f042ad7013b67ebd095af4d320305816c494e89745`.
The installed owner and bridge remain at checkpoint 100. The library has
no libdrm dependency.

Selected runtime source is approximately **14,838,627 bytes**, 7,025 more
than checkpoint 102's selected source inventory. This remains a partial
source estimate, not a complete kernel plus userspace model-token count;
the 3–5 MB target is unmet. The supported barrier covers one whole-color
image outside a render pass with a limited set of stages, access masks,
and layouts. It does not implement general Vulkan memory synchronization,
queue-family transfer, other image formats, or conformance.
