# GPU rendering directly into a native Vulkan image BO

The N100 owner now accepts a version-15 draw request that names a distinct,
owner-allocated render-target resource. The owner resolves its page-aligned
GPU address, patches the approved linear BGRA surface state, initializes the
target to the existing poison pattern, and verifies completed pixels from
that same resource. Vulkan's default path writes the image BO with the GPU
and skips the post-draw resource write ioctl. The owner still copies pixels
into the diagnostic ioctl response, so this is direct GPU targeting, not an
end-to-end zero-copy or GPU-clear implementation. No base-kernel ABI or Linux
graphics interface was added.

The physical N100 passed the opt-in matrix in `physical-matrix.log`: **39
cases, 124 draws** across 32×32 and 64×64 shapes, RGB, rotated and depth
vertex programs, binding offsets and `firstVertex`, second-page vertices,
and three FragCoord-red scales. `supplemental-matrix.log` adds **9 cases,
30 draws** for color ranges, named modules, two command buffers, two
pipelines, two render passes, and color-only image usage. Every draw was
version 15 with `direct_target=1`, `write_ioctl=0`, matching completion and
pixel hashes, and `fault=0`. The triangle probe independently reads back the
image BO and checks its full image hash and background.

After making direct targeting the default, `default-check.log` records four
default cases (16 draws) and two `RENKU_VK_DIRECT_TARGET=0` fallback cases
(8 draws), all passing. Run `python3 verify.py` to audit the saved traces.
The old owner rejected version 15 as expected (`old-owner-rejection.log`).
The Haiku VM owner build, packet smoke, Vulkan compiler/API/fence/reset
probes, and final library build passed (`owner-vm-build.log` and
`vulkan-vm-build.log`).

`ab-profile.json` records an A/B/A/B physical run with 32 repeated 64×64
draws per run. Across 64 draws per path, median host-observed submit time
was **5,494 µs** with the copy path and **5,455 µs** with direct targeting.
The post-draw write ioctl changed from a median **12 µs** to zero. These
measurements include CPU and GPU work, and their small total difference is
not evidence of an isolated GPU speedup.

Final installed/staged SHA-256 values:

| Binary | SHA-256 |
|---|---|
| Installed `intel_memory_owner` | `12c5bac49dfc75089bd4d01a096b382c59f09ea8c88bb07b6ed9578f67817559` |
| Installed `intel_native_execution_bridge` | `b0e6f27737103d4f7d150168ec6c42bec8412578924d014c28863ddab10d6e88` |
| Final default `libvulkan.so` | `c7afcefe1a18fb7fd6d367dd3ebb6446e4270d4d46cede3d0b1be58537560462` |
| `triangle_probe` | `074e15ff0b5db9f1e03ec6f81d43ea6b068ee9309d3965e04a44ba3ac02a460b` |

The previous installed owner, bridge, and Vulkan library are preserved in
`/boot/home/native-direct-target-053/rollback/` on the N100. Reproduce a
second-page rotated draw with the final default library:

```sh
LIBRARY_PATH=/boot/home/native-direct-target-053:/boot/home/native-fragcoord-048:/boot/home/native-rcs-large-039/vulkan-deps:/boot/home/config/lib:/boot/system/lib \
RENKU_VK_TARGET_64=1 RENKU_VK_TRACE=1 \
/boot/home/native-direct-target-053/triangle_probe 0 original 4 rotatepage
```

This remains a bounded Vulkan implementation: fixed 32/64 linear BGRA
targets, one vertex format, approved shader programs, and synchronous
submission. Direct targeting removes a copy boundary; it does not establish
Vulkan conformance or general presentation support.
