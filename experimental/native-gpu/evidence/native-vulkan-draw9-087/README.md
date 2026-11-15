# Multi-primitive Vulkan draw and page boundary, checkpoint 087

`vkCmdDraw` now records a nonzero multiple of three vertices as triangle-list
primitives when every 48-byte triangle fits the bound vertex buffer and one
4 KiB page. The validation runs before any draw record is appended. A single
call can therefore record three or more triangles; the queue uses the existing
two-triangle native batch for the first eligible pair and preserves the image
for later single-triangle submissions. This remains a narrow Vulkan path:
one instance, the existing vertex layout, and supported shader profiles.

The command-recording probe first failed with code 27 against checkpoint 086's
six-vertex limit. It now checks nine vertices (three records), fifteen vertices
(five records and allocation growth), a 48-byte undersized buffer, and a
48-byte triangle crossing a page boundary. Two complete triangles placed on
opposite sides of a page boundary are accepted.

The new `draw9` probe submits nine vertices in one `vkCmdDraw`. A separate
`draw3x3` probe submits the same vertices in three established calls. On the
physical N100, both produced exactly the same 32×32 and 64×64 images, with
final hashes `cdad4a41` and `9bd44344`. The first two triangles used one
version-20 RCS submission; the third used a preserving version-19 submission.
The raw readbacks and `run-hardware.py` preserve the comparison.

This work also found a real boundary failure. A 96-byte version-20 batch
cannot cross a 4 KiB page even when its two individual 48-byte triangles fit
on their respective pages. The pre-fix library tried version 20 at vertex
offset 4048; the owner rejected the ioctl before execution (`pre-fix-failure.log`).
The batch selector now requires the whole 96-byte pair to fit one page. On
both target sizes, the corrected library used version 15 at offset 4048 and
version 19 at offset 4096. Both completed, and their full readbacks matched
checkpoint 084's two-triangle images byte for byte.

The Haiku VM compiler, Vulkan API, fence, semaphore, and command-reset probes
passed after the source change. `run-prior-regression.py` replayed the **222
prior regression draws**, **24 image-offset draws**, and **16 memory-offset
draws** on the N100 against the final isolated candidate. All passed. The
installed owner and bridge were unchanged. The tested candidate library is
`/boot/home/native-draw9-087/libvulkan.so`, SHA-256
`755a3830c6bc243ed7f8d00b650a139aab3fa514c4db2b42e24d671ac89f48fc`.

The selected runtime source estimate is **15,230,510 bytes**, a 108-byte
increase from checkpoint 086's delta estimate. This is not a rebuilt Haiku
dependency closure or a model-tokenizer measurement. The 3–5 MB target is
still unmet.
