# First native Vulkan ABI slice on N100, 2026-11-14

The isolated native library built with RenkuOS `g++` and exported standard
Vulkan entry points. The [probe application](../../vulkan/api_probe.cpp)
linked to that library and identified the physical N100 as vendor `0x8086`,
device `0x46d1`. It created one graphics device and queue, created and
destroyed two shader modules from validated SPIR-V fixtures, and rejected a
requested unsupported geometry-shader feature. A red test before that
feature check exited 16; after implementation the probe passed. The library
and probe SHA-256 values are in [binaries.sha256](binaries.sha256).
Sixteen additional [probe runs](probe-repeat.log) also passed.

The same Mesa 26.3.0 source archive used by the shader compiler supplied
three `include/vulkan` headers and twelve `include/vk_video` headers. The
host extraction script verified archive SHA-256
`91d729aa68bb89e4ec857841300bbe922bcb3e52b1f97566ca83930d3b28343a`.
The headers compile on RenkuOS without Linux graphics interfaces and retain
their Khronos notices.

The two saved GCC depfiles and [hash manifest](manifest.json) give this
direct compile-input inventory:

| Category | Files | Bytes |
| --- | ---: | ---: |
| Khronos headers | 15 | 1,459,702 |
| Native Vulkan library source | 1 | 10,088 |
| Native render ABI header | 1 | 3,299 |
| Probe source and generated SPIR-V fixture | 2 | 6,966 |
| **Total** | **19** | **1,480,055** |

This is an input-file inventory for the current userspace library and probe,
not a complete Vulkan stack or model-token measurement. It excludes the
kernel owner, Mesa shader compiler, build-only scripts, and future pipeline,
memory, command, and presentation implementations. The development-model
tokenizer is unavailable, so the manifest leaves token fields null. These
figures are source bytes, not binary size or compression results.

No GPU rendering was submitted by this test. The library is experimental and
does not claim Vulkan 1.0 conformance. The installed RCS v6 kernel add-ons
were neither changed nor rebooted for this test.
