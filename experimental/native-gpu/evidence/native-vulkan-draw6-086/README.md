# One-call six-vertex Vulkan draw, checkpoint 086

`vkCmdDraw` now accepts either three or six vertices for the supported
triangle-list pipeline. A six-vertex call records two adjacent triangle
records atomically after checking the full 96-byte vertex range and page
boundary. The existing queue path combines those records into one version-20
native RCS submission when its batch conditions hold. Disabling batching with
`RENKU_VK_NATIVE_BATCH=0` uses the existing version-15 and version-19
two-submission path.

The new `triangle_probe ... draw6` mode supplies six vertices and calls
`vkCmdDraw(command, 6, 1, 0, 0)` once. Before the implementation, it failed
at `vkEndCommandBuffer` with code 25. The VM compiler-backed build then passed
the compiler, API, fence, semaphore, and command-reset probes. The reset probe
also checks two recorded offsets (0 and 48), image-preservation flags,
rejection of a 48-byte buffer, and rejection of a 96-byte range crossing a
4 KiB page.

On the physical N100, both 32×32 and 64×64 targets completed **one version-20
ioctl with two GPU triangles**. The final readbacks have hashes `d23b1ba1`
and `bb1f873b` respectively, with zero byte differences from checkpoint 084's
independent two-draw captures. The owner checked 585/585 and 3016/3016 pixels.
The six-vertex fallback also passed at both sizes with two ioctls, and the
prior two-call batch mode still passed. `run-hardware.py`, `hardware.log`, and
the six captured images preserve the reproduction and raw readback evidence.

`run-prior-regression.py` replayed checkpoint 084's physical probes against
the isolated candidate library and probe: **222 prior regression draws**, plus
**24 image-offset** and **16 memory-offset** draws. Every case passed. The
candidate was built in the Haiku VM and copied to
`/boot/home/native-draw6-086` on the N100. The installed owner and bridge
were unchanged; their tested SHA-256 values remain
`486d9f3d5476a266305085280f1476c217d2e92d1e31dbf8629aab2aef08c82b`
and `837385d1f4e7742d09dc133a2b1432d0bf22f47554a3c57dceccaf0a60c92b47`.
The candidate `libvulkan.so` SHA-256 is
`00c7bb68ffd0e29efb6476b27d1acbfe6ec3805c8b93ad323eb67707725a5fa5`.

The selected runtime source estimate increases by 296 bytes from checkpoint
084 to **15,230,402 bytes**. This remains a delta estimate based on the prior
host-derived closure, not a full Haiku dependency closure or a tokenizer
measurement. The 3–5 MB target remains unmet.
