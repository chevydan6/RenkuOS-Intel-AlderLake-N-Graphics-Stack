# Native N100 64×64 RCS draw

This increment adds a separate version-9 render ioctl that returns a 64×64
BGRA image across four 4 KiB pages. The existing render ioctl and its 4 KiB
request remain unchanged. The owner uses the Mesa 26.3.0 generated RCS packet
image, the previously mapped private PPGTT pages, and the existing RCS
submission and completion path. It verifies 2,582 pixels per orientation and
hashes all 4,096 readback pixels before returning the image.

The physical Intel N100 (PCI ID 8086:46d1, revision 0) booted with the owner
and bridge hashes in `physical-hashes.log`. After a read-only probe and RCS
bootstrap, all four 64×64 orientations passed. A second physical run repeated
each orientation four times (16 submissions), followed by the older fixed,
resource-vertex, and combined draws. All passed; see `physical-repeat.log`.
The physical Vulkan probe also passed its vertex-rotation, RGB, and depth
triangle cases under this owner. These remain 32×32 Vulkan tests; Vulkan has
not yet been extended to expose a 64×64 image.

The prior bridge rejected the new ioctl before GPU submission, establishing
that the large request was not silently routed to the old path. The Haiku VM
compiled the owner, bridge, and probe; the VM did not provide GPU execution
evidence. The Vulkan probe needed its prebuilt Mesa-derived compiler library
and `libSPIRV-Tools-shared.so` staged on the N100 to launch. No compiler source
was changed in this increment.
