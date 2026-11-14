# Native Vulkan buffer and memory slice on N100, 2026-11-14

The isolated RenkuOS `libvulkan.so` now creates a native owner-backed BO for
`VkDeviceMemory`, binds a vertex `VkBuffer`, and transfers mapped staging bytes
through the existing native resource ioctl. The probe writes twelve fixed
triangle-vertex dwords, explicitly flushes them, zeros its staging region,
invalidates from the BO, and checks all 48 bytes exactly. Its negative case
rejects binding a one-byte allocation to a 48-byte buffer and mapping beyond
the requested allocation. The owner BO rounds storage to a page; Vulkan
allocation bounds use the caller's requested size.

The first [physical log](fresh-boot.log) starts with a read-only RCS probe
after a clean reboot, then the native Vulkan probe, a v6 public-shader draw
with 654/654 expected pixels, and another read-only probe. `vkCreateDevice`
bootstrapped the owner without a separate manual bootstrap step. The next
[repeat log](repeat-and-draw.log) records 64 Vulkan buffer sessions followed
by another v6 draw. The requested-size negative test initially failed with
probe exit 26 against the rounded-size implementation; after the fix, a
second [repeat log](negative-repeat.log) records 64 passing sessions followed
by a v6 draw. The final memory-property correction removed an inaccurate
host-coherent flag and passed the physical probe again. All tests used the
installed v6 owner and bridge, with no base-kernel modification.

The final library SHA-256 is
`481cec69e425a6447bec3655e3466f1679763d535ede0b19ef8df8a460688cc2`;
the final probe SHA-256 is
`f2772af6ba5dafba852ac08728d133002730d4f6a8b2cf56a06741fe6b0cd98a`.
The exact physical paths and hashes are in [binaries.sha256](binaries.sha256).
The two GCC depfiles and [hash manifest](manifest.json) inventory the final
direct compile inputs:

| Category | Files | Bytes |
| --- | ---: | ---: |
| Khronos headers | 15 | 1,459,702 |
| Vulkan library source | 1 | 17,814 |
| Native render and resource ABI headers | 2 | 4,543 |
| Probe source and generated SPIR-V fixture | 2 | 9,377 |
| **Total** | **20** | **1,491,436** |

This measures the current userspace library and probe inputs. It excludes
the owner, bridge, Mesa shader compiler, build-only generators, and future
Vulkan pipeline and command code. Model-token count is unmeasured. This
library still does not submit a Vulkan draw or claim conformance.
