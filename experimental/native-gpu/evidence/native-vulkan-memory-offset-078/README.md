# Vulkan buffer memory binding offsets, checkpoint 078

The physical Intel N100 rendered triangles with vertex and uniform buffers
bound at byte offset 4096 in their `VkDeviceMemory` allocations. The new
probes first failed against the previous Vulkan library at
`vkBindBufferMemory` (application codes 10 and 67; see `red.log`). The
candidate passed at both 32×32 and 64×64. Vertex draws used both direct
image resources (owner version 15) and the copy fallback (version 13).
Uniform draws used owner version 17 with `uniform_offset=4096`.

`run-memory-offset.py` records eight cases and 16 draws, including the
existing descriptor-offset case. Every draw completed with matching GPU
sequence numbers, matched all 777 or 3476 checked pixels, and reported no
GPU fault or CSB error. The final 64×64 UBO image is
`ubo-memoffset64.bin` (16,384 bytes, SHA-256
`b1de60eb854ba90e72639b092820166918e2c363698e7facfaa136a478246eec`).

The candidate passed the VM compiler/API, fence, semaphore, and command-reset
probes and 222 existing physical regression draws: 124 ordinary, 28
semaphore, 16 green-gradient, 30 supplemental, and 24 arena. The owner and
bridge are unchanged from checkpoint 077. This userspace candidate is
isolated at `/boot/home/native-ubo-078`; `physical-hashes.log` identifies
its binaries.

To reproduce the hardware cases:

```sh
python3 experimental/native-gpu/evidence/native-vulkan-memory-offset-078/run-memory-offset.py
```

`stack-manifest.json` measures **12,663,152 unique bytes** of selected
canonical runtime source. SDK, generated files, and probes are excluded;
broader kernel/build inputs remain unmeasured. Model-token count is also
unmeasured. The 3–5 MB target remains unmet, and this is not a Vulkan
conformance claim.
