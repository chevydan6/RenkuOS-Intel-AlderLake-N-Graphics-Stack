# Native Vulkan preserving indexed pair, N100 checkpoint 095

Request v23 lets a bounded indexed triangle preserve the existing render
target. The first indexed draw uses v22; the second uses v23 with the same
vertex, index, and image BOs. The Vulkan command recorder accepts the second
indexed draw after a prior draw, or an indexed draw in a LOAD render pass.
The owner checks the same buffer ranges and index bounds as v22, stages each
index triplet in private GPU memory, and emits one GPU draw per submission.
No Linux DRM/GEM interface was added.

The physical N100 rendered two selected triangles from raw vertices
`[A,C,B,D]` and 16-bit index triplets `[0,2,1]` and `[0,1,3]` at offsets
0 and 6. The complete 32×32 and 64×64 pixel buffers matched a one-submission
nonindexed six-vertex reference byte for byte on three paired runs per size.
The final 32×32 image has FNV-1a `261a8285` and SHA-256
`25a9432f9a19c4f68a74a6b02b1679f85990f5257af9b82d155097509e5efd61`.
The 64×64 image has FNV-1a `554d3ec5` and SHA-256
`3bf868304fc2fdd05c9e50f2ef93ce3ccf47f5bb011d2c3c8cc56e01e7e24409`.
Owner traces show versions 22 and 23, increasing completed sequences, no
faults, and matched pixel checks. With direct target disabled, the Vulkan
driver rejects the pair before owner submission. `run-pair.py`,
`pair-hardware.log`, and the raw captures reproduce and preserve this proof.

After the v23 owner and bridge were installed and the N100 rebooted,
`run-prior-regression.py` passed **262** earlier physical draws. The prior
indexed regression passed 18 indexed draws, two nonindexed references, and
negative range, alias, and direct-target cases. The VM owner build, packet
smoke, compiler pipeline probe, and Vulkan API/fence/semaphore/command-reset
probes passed. The exact physical binary hashes are in `loaded-images.txt`;
known-good owner and bridge copies remain in
`/boot/home/native-indexpair-095/rollback-*`.

`measure-pair.py` ran 20 interleaved physical repetitions per mode and size.
It sums each process's traced blocking native submission durations, which
include the owner ioctl and wait. Medians were 10,090.5 us (two indexed
submissions) versus 4,838 us (one nonindexed submission) at 32×32, and
10,171.5 us versus 4,906.5 us at 64×64. These are neither frame latency nor
isolated GPU execution times. `pair-submit-times.json` retains every sample.
The extra owner submission is the next measured latency target; these data
do not isolate the per-draw GPU cost.

The selected canonical runtime-source inventory is **14,813,081 unique
bytes**, 367 bytes above checkpoint 094. It includes the pinned Mesa 26.3.0
Intel compiler closure and host SPIRV-Tools shared-target source; it omits
build-only generated files, SDK headers, probes, and RenkuOS kernel sources
outside the owner closure. It is neither a complete system source total nor
a development-model tokenizer count. The 3–5 MB design target remains unmet.

This is a bounded Vulkan capability. The pair uses two owner submissions,
one 16-bit index triplet per submission, a 32×32 or 64×64 direct target, and
the exercised shader profile. An indexed draw in a LOAD pass was accepted by
the code path but was not separately tested here. General index counts,
broader formats, presentation, and Vulkan conformance remain open.
