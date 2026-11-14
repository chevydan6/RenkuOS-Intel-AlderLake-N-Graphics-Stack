# Native Vulkan 64×64 N100 rendering

This increment extends the native Vulkan image path from 32×32 to 64×64.
The separately versioned render request (version 10) accepts file-local
vertex and fragment shader resources, validates them in the owner, builds
the 64×64 RCS packet image, and returns four pages of BGRA pixels. The
Vulkan driver writes those pages to its bound image resource and reads them
back through the existing native resource interface. No Linux graphics
kernel interface is involved.

The final physical Intel N100 run is in `physical-final.log`; installed
binary hashes are in `physical-hashes.log`. After probe and RCS bootstrap,
four triangle shapes with the original fragment shader and four with the
Mesa-compiled RGB fragment shader each passed two Vulkan submissions: 16
version-10 draws total. Every draw completed with zero reported fault and
matched the full 4,096-pixel image hash. The two direct version-10 resource
draws, version-9 fixed 64×64 draw, 32×32 Vulkan RGB/rotated-vertex/depth
draws, and version-7 combined draw also passed.
`physical-regression.log` additionally records all four version-1 fixed
draws, all four version-9 fixed 64×64 draws, the resource vertex draw, and
the combined draw under the final owner.

The VM built the owner, bridge, Vulkan library, and probe; its build logs
are saved here. GPU execution evidence comes from the physical N100. The
current 64×64 path supports the bounded passthrough vertex program and
selected fragment programs; 64×64 custom vertex programs are rejected at
submission. Rendering still stages and copies the image through CPU-visible
memory. This is a functional native Vulkan increment, not Vulkan
conformance. The 16 measured version-10 render ioctls took 5,132–5,794 µs
(median 5,275 µs), including submission, completion, validation, and
readback; these numbers are not isolated GPU execution times.
