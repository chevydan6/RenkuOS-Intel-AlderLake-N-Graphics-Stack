# Fixed Vulkan image object on N100, 2026-11-14

The direct-linked native Vulkan library now accepts one 32×32 linear BGRA
`VkImage` shape with color-attachment and transfer-source usage. It reports a
4 KiB, page-aligned memory requirement, binds an existing owner-backed
`VkDeviceMemory`, and rejects a 33-pixel-wide image. The test first failed at
link time for the five missing image entry points, then passed after their
implementation. Sixteen consecutive [physical N100 probes](repeat-and-draw.log)
passed, followed by the established v6 public-shader hardware draw with
654/654 expected pixels. The owner and base kernel were unchanged.

Final library SHA-256:
`2d52fdd2001b20ac6ad4ed9353c73c81148c795c6d6c95aa053867a88f5acdfc`.
Final probe SHA-256:
`6c7e1b9fd099a10c4bb0bb6e316fc1dcbca6b310a1e1bde740136c80634f7e4c`.
See [binaries.sha256](binaries.sha256) for their physical paths.

The final [direct compile-input manifest](manifest.json) derives from the two
saved GCC depfiles:

| Category | Files | Bytes |
| --- | ---: | ---: |
| Khronos headers | 15 | 1,459,702 |
| Vulkan library source | 1 | 20,304 |
| Native render and resource ABI headers | 2 | 4,543 |
| Probe source and generated SPIR-V fixture | 2 | 10,685 |
| **Total** | **20** | **1,495,234** |

This is a fixed-shape object and allocation milestone, not a Vulkan image
rendering result. The owner currently returns pixels in its render ioctl
output, and Vulkan has not submitted a draw or filled this image. The manifest
does not include the kernel owner, Mesa compiler, build generators, or model
tokens.
