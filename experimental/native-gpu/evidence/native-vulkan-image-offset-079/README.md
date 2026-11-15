# Vulkan image memory binding offsets, checkpoint 079

The physical Intel N100 rendered into a Vulkan image bound at byte offset
4096 in its `VkDeviceMemory` allocation. The previous Vulkan library
rejected that binding at application code 16 (`red.log`). The new owner
request version 18 carries the page-aligned image offset for direct target
rendering. The userspace copy fallback writes to the same bound image range.

`run-image-offset.py` passed 12 cases and 24 draws at 32×32 and 64×64:
direct rendering, the copy fallback, an approved rotated vertex shader,
and UBO colors with uniform offsets 0, 16, and 4096. Every draw had a
matching completion sequence, matched all 777 or 3476 checked pixels, and
reported no GPU fault or CSB error. The probe also wrote a 4 KiB guard
page before the image range and verified it remained unchanged after each
case. The final 64×64 image is `image-offset64.bin` (16,384 bytes, SHA-256
`b1de60eb854ba90e72639b092820166918e2c363698e7facfaa136a478246eec`).

The VM owner/bridge build and Vulkan compiler/API, fence, semaphore, and
command-reset probes passed. The candidate passed 222 existing physical
regression draws: 124 ordinary, 28 semaphore, 16 green-gradient, 30
supplemental, and 24 arena. The preceding buffer-memory-offset cases
also passed against the new owner (16 draws), as did the original
version-16 UBO path at both sizes (4 draws). The installed owner and
bridge and the isolated userspace bundle `/boot/home/native-ubo-079` are
identified in `physical-hashes.log`; the prior owner/bridge pair is
retained in that bundle's `rollback/` directory.

To reproduce the new cases:

```sh
python3 experimental/native-gpu/evidence/native-vulkan-image-offset-079/run-image-offset.py
```

`stack-manifest.json` measures **12,664,657 unique bytes** of selected
canonical runtime source. SDK, generated files, and probes are excluded;
broader kernel/build inputs and model-token count remain unmeasured. The
3–5 MB target remains unmet. No Vulkan conformance claim is made.
