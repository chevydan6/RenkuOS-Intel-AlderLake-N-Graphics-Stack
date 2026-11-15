# N100 last-page shader execution (070)

The `endva` probe allocates a 60-page padding BO, then places its vertex,
image, and fragment-shader BOs at the far end of the 64-page resource arena.
This tests the full address range opened in checkpoint 069. It uses the same
installed owner, Vulkan library, and Intel compiler; only the test probe
changed. No base-kernel or owner code changed.

The physical N100 passed [four direct draws](direct.log) and [four fallback
draws](fallback.log). Each completed without an ioctl or GPU fault and matched
the 32×32 expected pixel hash `28616d2d`. The vertex buffer was at `0x44c000`,
the image at `0x44e000`, and the fragment shader at `0x44f000`, the final page
of the prebound arena. Run [verify.py](verify.py) to check all eight saved
structured traces. [Physical hashes](physical-hashes.log) identify the tested
binaries; [VM build output](vm-build.log) records the probe build.

The VM source bundle is preserved at
`/boot/home/test-work/vulkan-endva-070`; the checkpoint 069 bundle was restored
to its original probe source so its measured manifest remains reproducible.
The runtime source closure remains 12,637,371 bytes because this increment
only adds a test probe. Model-token count remains unmeasured.
