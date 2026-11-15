# N100 Vulkan six-index draw, checkpoint 096

Request v24 submits a six-index triangle list in one GPU primitive and one
owner ioctl. It selects four vertices from an application-owned vertex BO,
stages 12 validated index bytes from a separate index BO, and writes directly
to the application image BO. This replaces two blocking submissions for the
tested quad while retaining the earlier v22/v23 paths.

The physical N100 rendered the quad at 32×32 and 64×64. Six repeated full
pixel captures matched the independent nonindexed reference from checkpoint
095 byte for byte. Final FNV-1a hashes were `261a8285` and `554d3ec5`;
SHA-256 hashes were
`25a9432f9a19c4f68a74a6b02b1679f85990f5257af9b82d155097509e5efd61`
and `3bf868304fc2fdd05c9e50f2ef93ce3ccf47f5bb011d2c3c8cc56e01e7e24409`.
`run-six.py`, `six-hardware.log`, and the raw captures preserve the tests.
An out-of-range sixth index was refused by the owner at stage 0, without a
GPU sequence. A page-crossing index range was refused during command
recording. Disabling direct target rejected the request before submission.

The VM owner build, packet smoke, Mesa compiler pipeline, and Vulkan API,
fence, semaphore, and command-reset probes passed. The new owner and bridge
were installed after preserving the v23 binaries under
`/boot/home/native-indexsix-096/rollback-*`; the N100 rebooted and passed
all **262** prior hardware draws plus the earlier indexed regression.
`loaded-images.txt` records the exact installed hashes. The candidate
Vulkan library loads the native Intel compiler and no libdrm.

After three warmups and 20 interleaved repetitions per mode and size, median
blocking owner submission time was 4,909 us for the six-index draw versus
10,196.5 us for the two-submission indexed pair at 32×32. At 64×64, the
medians were 5,031.5 us versus 10,189 us. The one-submission nonindexed
reference measured 4,883.5 and 5,075 us. These timings include the owner
ioctl and wait; they are not isolated GPU time or frame latency. All samples
are in `six-submit-times.json`.

The selected canonical runtime-source inventory is **14,814,188 unique
bytes**, 1,107 bytes above checkpoint 095. This includes the pinned Mesa
26.3.0 Intel compiler and SPIRV-Tools shared-target source, but excludes
build-only generated files, SDK headers, probes, and RenkuOS kernel sources
outside the owner closure. It is not a complete system source total or a
development-model token count. The 3–5 MB target remains unmet.

This is still a bounded Vulkan draw: exactly six 16-bit indices, four vec4
vertices, the exercised constant-color shader profile, and a 32×32 or 64×64
direct target. General indexed meshes, arbitrary shaders, presentation,
and Vulkan conformance are unfinished. The next capability should widen
the application-visible rendering contract rather than add another fixed
geometry fixture.
