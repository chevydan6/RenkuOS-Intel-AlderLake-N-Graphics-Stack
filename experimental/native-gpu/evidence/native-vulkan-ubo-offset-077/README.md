# Vulkan UBO descriptor offsets, checkpoint 077

The physical Intel N100 rendered two triangles at each of 32×32 and 64×64
using a Vulkan uniform-buffer descriptor at byte offset 16. The first 16
bytes of the same buffer contained a different color. The Vulkan library
sent owner request version 17; the owner resolved the exact 16-byte range
and patched binding-table entry 1 to its GPU address. Each 32×32 draw
matched 777/777 checked pixels; each 64×64 draw matched 3476/3476.
Completion sequences matched, and no GPU fault or CSB error was reported.

The new `ubooffset` probe failed at command-buffer completion against the
previous Vulkan library because it rejected nonzero descriptor offsets.
With the candidate library and owner, `run-ubo-offset.py` passed both sizes.
The final 64×64 image is `ubo-offset64.bin` (16,384 bytes, SHA-256
`b1de60eb854ba90e72639b092820166918e2c363698e7facfaa136a478246eec`).
The final pixel hash is `8df12ff5`. The Vulkan trace now records the
descriptor's `uniform_offset`.

The VM build passed the owner/bridge CPU suite and Vulkan compiler/API,
fence, semaphore, and command-reset probes. The candidate also passed the
222 existing physical regression draws: 124 ordinary, 28 semaphore,
16 green-gradient, 30 supplemental, and 24 arena draws. Version-16 UBO
draws still passed at both sizes. The physical candidate is isolated at
`/boot/home/native-ubo-077`; previous owner and bridge binaries are in
its `rollback/` directory. `physical-hashes.log` identifies the installed
owner, bridge, library, probe, and compiler binaries.

To reproduce the offset test:

```sh
python3 experimental/native-gpu/evidence/native-vulkan-ubo-offset-077/run-ubo-offset.py
```

`stack-manifest.json` measures **12,662,980 unique bytes** of selected
canonical runtime source. It excludes SDK, generated files, and probes.
It also leaves broader kernel and build-tool inputs unmeasured. The model
tokenizer is unavailable, so no token count or complete-source size is
claimed. The 3–5 MB target remains unmet. No Vulkan conformance claim is
made.
