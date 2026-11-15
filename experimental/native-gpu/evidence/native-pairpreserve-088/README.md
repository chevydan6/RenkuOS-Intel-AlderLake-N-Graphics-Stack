# Preserving two-triangle RCS batch, checkpoint 088

Owner request version 21 combines the existing two-triangle packet extension
from version 20 with version 19's direct-target preservation. It accepts the
same bounded constant-color shader and contiguous 96-byte vertex range as
version 20. Before submission, the owner snapshots the target image. Its
pair-image verifier checks the union of the new triangles against that
snapshot; the 32×32 and 64×64 CPU smoke tests cover unchanged background,
rendered interiors, and a corrupted pixel. Versions 19 and 20 retain their
previous behavior.

Vulkan selects version 20 for the first eligible adjacent pair in a render
pass and version 21 for later eligible pairs. A four-triangle draw now uses
**two RCS submissions** instead of the prior **three**. `draw12` records four
triangles in one `vkCmdDraw`; `draw3x4` records the same geometry in four API
calls. Both take versions 20 and 21 on the physical N100. The fourth triangle
was chosen to add pixels beyond the first three, so the final image detects
a missing second pair.

The old library and owner first rendered a baseline with versions 20, 19,
and 19. The new path's 32×32 and 64×64 readbacks match those baseline images
byte for byte. Final pixel hashes are `5bf758b9` and `61162dae`. The owner
reported 626/626 and 3152/3152 checked pixels for version 21. Disabling
batching selects versions 15, 19, 19, 19 and yields the same complete images.
`run-hardware.py`, `hardware.log`, and the raw images preserve this comparison.

`measure-four-triangles.py` interleaved 20 baseline and 20 new-path runs at
each size. Every run checked the full submission reports and final image hash.
Median CPU-observed blocking `submit_us`, summed across the draw's submissions:

| Target | Three submissions | Two submissions | Reduction |
| --- | ---: | ---: | ---: |
| 32×32 | 15,474 µs | 10,264.5 µs | 33.7% |
| 64×64 | 15,609.5 µs | 10,348 µs | 33.7% |

These numbers include CPU-side ioctl and wait costs for this workload; they
do not isolate GPU execution time. `samples.json` and `summary.json` retain
the individual traces and calculations.

The candidate owner and bridge passed the Haiku VM build and CPU smoke suite.
The compiler-backed Vulkan VM build passed its compiler, API, fence,
semaphore, and command-reset probes. After staging the old owner and bridge in
`/boot/home/native-pairpreserve-088/rollback`, the physical N100 was rebooted
with the new binaries. `run-prior-regression.py` then passed the **222 prior
regression draws**, **24 image-offset draws**, and **16 memory-offset draws**.
The installed owner SHA-256 is
`536e3839db0587eeab4316529bab22ac7c5a2be993ee94e98888de4b4d0c7e7f`,
the bridge is
`bfc3d1cd1f1bf90cb9788f770d90573b428880c88919b72caea10a77af755496`,
and the tested Vulkan library is
`b5fefb2d69c8e521500b7046140fb315b0f6a9c7e4018e21e23c205f7f1d1a28`.

Selected runtime source is estimated at **15,230,927 bytes**, 417 bytes above
checkpoint 087's delta estimate. This is still a host-derived selected-source
estimate, not a complete Haiku build closure or an actual model-token count.
The 3–5 MB source target remains unmet.
