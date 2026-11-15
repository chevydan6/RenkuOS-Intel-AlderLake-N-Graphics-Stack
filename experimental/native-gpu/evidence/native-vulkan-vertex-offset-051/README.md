# Native Vulkan vertex offsets on the physical N100

The native Vulkan path now records the `vkCmdBindVertexBuffers` offset and
`firstVertex` for a three-vertex draw. The version-13 owner request carries a
16-byte-aligned byte offset into the existing vertex resource BO. The owner
resolves the selected 48-byte range, checks that it stays within one mapped
page, validates those vertices, and patches the vertex-buffer packet address.
This adds no base-kernel ABI and makes no Linux graphics call.

The test buffer contains a wrong triangle at byte zero and the expected
triangle at byte 48 or 4096. A draw using the wrong address would fail the
probe's full-image hash and background check. Nonzero offsets with a distinct
vertex shader remain outside the current version-13 workload.

## Physical result

`physical-matrix.log` contains **34 cases and 112 completed draws** on the
Intel N100. It covers existing 32×32 and 64×64 triangle shapes and RGB,
rotated and depth vertex shaders, the three FragCoord-red scales, binding
offset 48, `firstVertex=3`, and a 4096-byte binding offset into an 8192-byte
VkBuffer. Every submission completed with `ioctl=0`, `fault=0`, matching fence
sequence, and `matched=checked` pixels. The probe independently checked the
expected full-image hash after each draw. Run `python3 verify.py` here to audit
the saved case counts, versions, offsets, completion, and hashes.

The red tests are retained: `old-owner-rejection.log` shows version 13 rejected
by the previous owner; `old-page-limit-rejection.log` shows the first offset
implementation rejecting a second-page draw. The final owner accepted that
draw. `owner-vm-build.log` and `vulkan-vm-build.log` record the Haiku VM builds.
The host CPU packet smoke passed, including rejection of misaligned and
page-crossing vertex addresses.

Installed binary SHA-256 values on the physical N100:

| Binary | SHA-256 |
|---|---|
| `intel_memory_owner` | `8938de925de8c0962da63191a7b5174cd5baf8f1b91fce04ee274b9f6cce457d` |
| `intel_native_execution_bridge` | `8c4c3f8dbcefe8ca0a9d4a27480be9ce53680d6d3b7d2ecfdcd94011aa33f801` |
| `libvulkan.so` | `516ced7a735c2bbcc79e0f7a7ceb2899980876f96bae9af65dd5a3c86d324754` |
| `triangle_probe` | `0307b707c19165a000a0d3fdb0022940c5b1d21e80d821bbaac2fa60fe03c126` |
| `librenku_intel_compiler.so` | `973ac0a931be4210cae5925e366c04c55331c065f9527cfcce5560c017f870ea` |

Reproduce a second-page draw on the N100 with the installed bundle:

```sh
LIBRARY_PATH=/boot/home/native-vertex-offset-051:/boot/home/native-fragcoord-048:/boot/home/native-rcs-large-039/vulkan-deps:/boot/home/config/lib:/boot/system/lib \
RENKU_VK_TARGET_64=1 RENKU_VK_TRACE=1 \
/boot/home/native-vertex-offset-051/triangle_probe 0 original 4 pageoffset
```

The pre-change owner and bridge are preserved in
`/boot/home/native-vertex-offset-051/rollback/` on the N100. The final
installed owner and bridge hashes above were checked after reboot. Vulkan
feature coverage remains narrow: this does not imply general vertex formats,
arbitrary shader combinations, or Vulkan conformance.
