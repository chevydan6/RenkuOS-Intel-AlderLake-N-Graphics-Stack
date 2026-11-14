# Compiler-backed Vulkan vertex BO through owner version 8

Mesa 26.3.0 compiles `triangle-depth-quarter.vert.spv` to the previously
validated 192-byte Intel N100 vertex program. `CompiledKnownShader.h` now
accepts that exact program, the original 128-byte vertex program, and the
bounded RGB fragment program. The depth fixture header reproduces the 412-byte
SPIR-V file byte for byte (SHA-256
`26fae11aaa821e327c4cf7c84aac8a59a2a6324f9182944749b67d072bf4814b`).

The Vulkan pipeline allocates and writes a third resource for the 192-byte
vertex program, then submits render request version 8 with that file-local
handle in the first four input bytes of `output`. The original vertex program
continues to use version 7. Both paths use the same native owner and image
readback. Vulkan traces include the vertex-shader BO handle.

The VM compiler/API/fence/reset probes and build passed (`vm-build.log`). The
physical N100 ran the updated library SHA-256
`29b2a9704874acd4dea73d3914e833781729692feb07b146708f985ec1ac34ec`
and triangle probe SHA-256
`be1c7c7f1867e030340d3021dcec847492561b3a15d868610f1d713b5cb6b629`
from `/boot/home/native-vulkan-depth-029/`. No installed owner or bridge
binary was replaced. The physical run produced four version-8 submissions,
owner sequences 17–20, all complete with no fault and exact shape hashes
`6f2bc92d`, `a4e13743`, `ce445d05`, and `fffe3dc5`. The original version-7
named-vertex and two-draw batch regressions passed (sequences 21–23). Raw
structured traces are in `physical.log`.

The depth shader writes vertex z=0.25 while depth testing is disabled. Matching
pixels therefore prove the Vulkan resource, submission, completion, and
readback path but do not independently prove execution of the changed vertex
arithmetic. A visible x/y transform with independent expected pixels is the
next hardware proof.
