# Native Vulkan vertex shaders at 64×64

Version 11 extends the native large-image request with a distinct file-local
vertex shader handle. The owner validates the complete shader resource page
against its approved machine-code programs, copies the selected program into
its private VS page, and checks the 64×64 output against the transformed
vertex positions. The Vulkan path selects version 11 when a compiled vertex
shader differs from the passthrough program. No Mesa compiler source changed.

On the physical Intel N100, the compiled half-turn vertex program rendered
the opposite triangle at 64×64 with full-image hash `7b48f3e5`; the depth
vertex program retained the original 64×64 triangle with hash `8aaee2f5`.
Each program passed eight repeated Vulkan submissions. All 16 version-11
reports showed completed sequences, matching checked pixels, and zero GPU
faults. The original and RGB fragment programs across four 64×64 triangle
shapes, 32×32 rotated/depth/RGB draws, and direct RCS resource, fixed, and
combined draws also passed. See `physical-final.log` and the installed
binary checksums in `physical-hashes.log`.

The Haiku VM built the owner, bridge, Vulkan library, and probe; its logs are
saved here. GPU execution evidence comes from the physical N100. The 16
version-11 render ioctls took 5,253–5,548 µs (median 5,345 µs), including
submission, completion, validation, and readback. This remains an
experimental bounded Vulkan implementation; the measurements do not isolate
GPU execution time and do not establish Vulkan conformance.
