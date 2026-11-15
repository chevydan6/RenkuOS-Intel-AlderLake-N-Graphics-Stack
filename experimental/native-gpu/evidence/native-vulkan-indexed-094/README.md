# Native Vulkan indexed N100 triangle, checkpoint 094

Request v22 adds one genuine GPU indexed draw to the native owner. The Vulkan
driver exposes `vkCmdBindIndexBuffer` and a bounded `vkCmdDrawIndexed` path.
`vkGetPhysicalDeviceProperties` reports the exercised index and vertex-input
limits.
It accepts four `vec4` vertices in a public vertex BO and three 16-bit indices
in a distinct index BO. The owner checks handle separation, offsets, index
bounds, and the selected triangle before submission. It copies the six index
bytes to its private GPU page at `0x403100`, inserts the Gen12
`3DSTATE_INDEX_BUFFER` packet, and uses a random-access `3DPRIMITIVE`.
The GPU fetches the vertices and indices; the CPU does not expand indices
into a reordered vertex buffer. The private index copy is staging overhead,
not yet a zero-copy public index path.

The physical N100 completed the indexed draw at 32×32 and 64×64. The test
stores raw vertices in order `[A,C,B,D]` and draws indices `[0,2,1]`, so a
sequential vertex fetch cannot produce the expected triangle. Both indexed
images matched an independent nonindexed reference **byte for byte**:

| Size | First image FNV-1a | First image SHA-256 |
|---|---|---|
| 32×32 | `28616d2d` | `d078ce6ad13cc5e13e85f660b0ea53a2963a47ad619297d7796f82e1f82627eb` |
| 64×64 | `8aaee2f5` | `c3cd98385b4fc4dc40f92bec112697c8ecaabe670c029db25c6f18217591882d` |

The same command buffer was then submitted twice while the bound index BO
changed to `[0,3,1]` between submissions. Both draws completed with one GPU
draw, matched owner pixel checks, no fault, and increasing sequence numbers.
The second 32×32 image had hash `459d92c5` and SHA-256
`fe182d4ba6c6fa704fd4110a834ed74508614b2fc44f667f397fc9eff92ebcfb`;
the second 64×64 image had hash `5fdea115` and SHA-256
`f7b22aba25b8688ccfe24dcc38707de308242d27401861888dd0404c09f58d65`.
These second images have repeatable full-image hashes and owner pixel checks;
they do not have an independent nonindexed full-image oracle.

`run-indexed.py` passed three one-draw and three two-draw repetitions at
each size: 18 indexed GPU draws plus two nonindexed references. It also
verified that indices outside the four-vertex range and an index BO aliasing
the vertex BO are rejected at stage 0 without a GPU sequence. With the
direct-target path disabled, the Vulkan driver rejects the indexed submission
before calling the owner.
`indexed-hardware.log`, the captured images, and `loaded-images.txt` retain
the traces, raw output, and exact installed binary hashes. The known-good
owner and bridge were copied to `/boot/home/native-indexed-094/rollback-*`
before installation. The physical N100 rebooted and remained reachable.

The Haiku VM owner build, packet smoke test, compiler pipeline probe, and
Vulkan API/fence/semaphore/command-reset probes passed. After installation,
`run-prior-regression.py` passed all **262** earlier physical draws with the
new owner, Vulkan library, and checkpoint-092 compiler. The owner and bridge
are Haiku-native; no Linux DRM/GEM transport was added.

The selected runtime-source inventory is **14,812,714 unique bytes**,
2,119 bytes above checkpoint 092. It includes the pinned Mesa 26.3.0 Intel
compiler closure and host SPIRV-Tools shared-target source reproduction;
it excludes build-only generated files, SDK headers, probes, and RenkuOS
kernel sources outside this owner closure. It is neither a complete system
source total nor a development-model token count. The 3–5 MB design target
remains unmet. No indexed-versus-sequential performance claim is made from
these functional tests.

This is a bounded Vulkan increment: one indexed triangle must be the first
draw of a `DONT_CARE` pass, with one 16-bit index triplet, no vertex shader
variant or UBO, and a 32×32 or 64×64 direct target. General indexed draws,
broader formats, presentation, and Vulkan conformance remain open.
