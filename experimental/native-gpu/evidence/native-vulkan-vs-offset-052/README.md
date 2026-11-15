# Distinct vertex shader with a vertex buffer offset

Version 14 extends the owner-validated native draw request: input output
bytes 0–3 carry a nonzero, 16-byte-aligned vertex byte offset; bytes 4–7
carry an owner-approved distinct vertex shader resource handle. The owner
resolves and validates the selected 48-byte vertex range, fragment program,
and vertex program before publishing the GPU batch. No base-kernel ABI or
Linux graphics dependency was added.

The physical N100 passed `physical-matrix.log`: **15 cases, 60 draws**,
including 28 version-14 draws. The new cases combine binding offset 48,
`firstVertex=3`, or second-page offset 4096 with rotated or depth vertex
programs at 32×32 and 64×64. Existing offset, original shader, and
FragCoord-red paths were also repeated. The probe keeps a decoy triangle at
byte zero and checks the entire returned image against an expected hash and
background. The saved traces report matching completion sequences, exact
pixel counts, and no GPU fault. Run `python3 verify.py` to audit the trace.

The old Vulkan library rejected the new rotate-offset probe before
submission (`old-vulkan-rejection.log`). The new library reached the old
owner, which rejected request version 14 (`old-owner-rejection.log`). The
new owner and bridge were then installed and the N100 rebooted. The VM owner
build and CPU suite passed (`owner-vm-build.log`); the VM Vulkan compiler,
API, fence, and command-reset probes passed (`vulkan-vm-build.log`). The
command-reset fixture now declares the 48-byte vertex buffer it records,
and the build script includes its bundled SPIR-V library path.

Installed/staged SHA-256 values:

| Binary | SHA-256 |
|---|---|
| Installed `intel_memory_owner` | `36849f23b6c4467c2ab36c0565ee6d63f6d13d14e53f9bb7e494bd3f5251914f` |
| Installed `intel_native_execution_bridge` | `51d85f4331b4f0bd1bfddd108015a04cab36b660b4a4f4c690ecc721ee5d101c` |
| Bundle `libvulkan.so` | `ecc87f6b9fa7b6c97475e6f69ab551747838b349b62adb0df1337de98abc1c20` |
| Bundle `triangle_probe` | `421a13d26074e983282139cfde09758526cc2e5ef66a7e3a3fe07ab3840eabae` |

The previous installed owner and bridge are saved under
`/boot/home/native-vs-offset-052/rollback/` on the N100. Reproduce the
second-page rotated draw:

```sh
LIBRARY_PATH=/boot/home/native-vs-offset-052:/boot/home/native-fragcoord-048:/boot/home/native-rcs-large-039/vulkan-deps:/boot/home/config/lib:/boot/system/lib \
RENKU_VK_TARGET_64=1 RENKU_VK_TRACE=1 \
/boot/home/native-vs-offset-052/triangle_probe 0 original 4 rotatepage
```

This remains a bounded Vulkan experiment, with approved shader profiles,
one vertex format, and fixed draw and image shapes. It is not a Vulkan
conformance result.
