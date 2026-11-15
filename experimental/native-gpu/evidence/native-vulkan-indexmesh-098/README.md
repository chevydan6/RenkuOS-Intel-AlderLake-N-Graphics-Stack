# Native Vulkan indexed triangle lists, N100 checkpoint 098

Owner request v25 and the native Vulkan driver now accept a bounded indexed
triangle list from application vertex and index buffers: 3–256 vec4 vertices
and 3–255 16-bit indices, with an index count divisible by three. The GPU
fetches vertices from the public BO and staged indices from the owner's
private page. One random-access TRILIST primitive covers the full list.
The request can preserve an existing render target, including a CLEAR pass.
It keeps the existing native allocation, submission, and synchronization
interfaces; no Linux DRM/GEM transport was added.

The physical N100 executed a nine-index, nine-vertex mesh at 32×32 and
64×64. The raw vertex buffer deliberately reordered two vertices; the index
buffer selected the intended order. Both complete image captures matched
the independent nonindexed three-triangle reference from checkpoint 087
byte for byte. SHA-256 hashes were
`87d977b759fdcb13f13541c87a123c830ca177b9d46c51b2510c56451d878f1d`
and `3f45099273aee893f2430878c12fcd96a5cc345f1e680565b281b5217b1c7f62`.
A 32×32 CLEAR-plus-mesh pass also matched the reference with its background
replaced by `0xff336699` (SHA-256
`c68b5029e1becfed40d7d9a15ad0d3229196c64d7d717dd108689060d6491392`).
An index equal to the vertex count was refused at owner stage 0 with no GPU
sequence. One earlier six-index v24 draw passed after the v25 install.
`run-mesh.py`, `hardware.log`, and the three raw captures retain the targeted
hardware evidence.

The owner no longer uses a fixed CPU pixel oracle for v25. It reports
`pixel_oracle=0`, zero checked/matched pixels, a completed sequence, and a
full-image readback hash; the Vulkan driver verifies the hash and completion.
The application test compares every output pixel to an independent saved
reference. Existing v22–v24 requests retain their CPU pixel checks.
This distinction prevents the completion report from claiming that an
arbitrary image has been verified against an owner-side oracle.

The Haiku VM owner build, packet smoke, compiler pipeline, Vulkan API,
fence, semaphore, and command-reset probes passed. The N100 rebooted with
the v25 owner and bridge; their previous v24 binaries are preserved under
`/boot/home/native-indexmesh-098/rollback-*`. `loaded-images.txt` records
the exact installed hashes. The candidate Vulkan library has no libdrm
dependency. In line with the request to avoid excessive testing, the full
262-draw hardware suite was not rerun at this checkpoint.

The selected runtime-source inventory is **14,819,901 unique bytes**,
including pinned Mesa 26.3.0 Intel compiler and SPIRV-Tools shared-target
source. It excludes build-only generated files, SDK headers, probes, and
RenkuOS kernel sources outside the measured owner closure. It is not a
complete system source total or a development-model tokenizer count. The
3–5 MB target remains unmet.

This is a useful but bounded mesh path: one vertex vec4 input, one constant
color fragment shader profile, finite position values within the owner's
current range, one 32×32 or 64×64 linear BGRA8 target, one triangle list,
and no general presentation or Vulkan conformance claim. Only the nine-index
case was exercised on hardware here; the larger admitted counts need their
own workload coverage as applications begin to use them.
