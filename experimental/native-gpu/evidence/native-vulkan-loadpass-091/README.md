# Vulkan render-pass LOAD on the N100, checkpoint 091

The native Vulkan driver now accepts one color attachment with
`VK_ATTACHMENT_LOAD_OP_LOAD` when its initial and final layouts are
`VK_IMAGE_LAYOUT_GENERAL`. It records the load choice in the render-pass
object and marks the first draw of that pass as preserving the existing image.
The existing direct-target owner request version 19 snapshots and verifies
that image before drawing. `DONT_CARE` with an undefined initial layout keeps
its previous replacement behavior. A preserving draw still requires the
direct-target path; disabling it rejects the whole submission before any
owner draw.

The `loadpass` probe first draws one triangle in a `DONT_CARE` pass, then a
complementary triangle in a `LOAD` pass over the same image. This uses two
render-pass and framebuffer objects, each with its own compatible pipeline.
The old library rejected creation of the second pass (`red-old-library.log`,
exit 78). The patched Haiku VM build passed compiler, API, fence, semaphore,
and command-reset probes (`vm-build.log`). The API probe also rejects a LOAD
attachment whose initial layout is undefined.

On the physical N100, the 32×32 and 64×64 `loadpass` runs produced owner
request versions **15 then 19**, completed both submissions without faults,
and matched the full raw image from the equivalent two draws within one pass
byte for byte. The final image hashes are `225b1ba1` and `bb1f873b`. The old
two-pass `DONT_CARE` case still selected versions 15 and 15 and produced a
different image. A LOAD pass with direct target disabled returned before any
submission. `run-hardware.py`, `hardware.log`, and the five captures preserve
these results.

The candidate `libvulkan.so` was loaded from `/boot/home/native-loadpass-091`
alongside the checkpoint-090 compiler. `physical-images-and-hashes.log`
records the loaded images and installed hashes. After putting the candidate
first in `LIBRARY_PATH`, `run-prior-regression.py` passed all **262 previous
physical draws**: 124 base, 28 semaphore, 16 green-gradient, 24 arena, 30
supplemental, 24 image-offset, and 16 memory-offset draws. The installed
owner and bridge remained at checkpoint 088; no kernel code changed.

The measured selected runtime source inventory, including the pinned host
SPIRV-Tools reproduction, is **15,072,397 unique bytes**, 204 bytes above
checkpoint 090. `vulkan-manifest.json`, `stack-manifest.json`, and
`spirv-tools-closure.json` retain the file hashes and accounting. This is
still a selected runtime source inventory, not the complete Haiku build
closure or a development-model token count. The 3–5 MB target remains open.
