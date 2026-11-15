# Native Vulkan indexed mesh with fragment UBO, N100 checkpoint 104

The Vulkan path now combines a bounded 16-bit indexed triangle list with the
Mesa 26.3.0 compiled fragment shader that reads one 16-byte UBO. Owner command
version 26 carries the index and uniform resource handles separately. The
owner validates the exact fragment program, index range, vertex positions,
resource separation, uniform address, and uniform color before publishing
the GPU batch. The existing indexed batch and uniform surface packet patches
are composed; neither has a new GPU packet implementation.

The new `triangle_probe 0 ubo 1 indexmeshubo` failed at
`vkEndCommandBuffer` with exit code 25 against the installed checkpoint-103
library, before implementation. After building the owner, bridge, and Vulkan
library on the Haiku VM with `-Werror`, the N100 rebooted into the new owner
and bridge. The probe submitted one 9-index, 9-vertex draw. The RCS report
showed command version 26, completion stage 12, observed sequence 3,
`fault=0`, engine stage 4, and image hash `f2927141`. The reference indexed
mesh using the constant fragment shader had image hash `cdad4a41`.

The raw 32×32 BGRA images are in this directory. `verify-indexubo.py`
compares all 1,024 pixels: exactly 436 rendered pixels change from
`0xffff4000` to the UBO-supplied `0xff00ff80`, while 588 background pixels
remain `0x19537bad`. It reports zero mismatches. This proves the GPU
fragment path fetched the descriptor-backed uniform on the indexed mesh.

The VM compiler, API, fence, semaphore, and command-reset probes passed.
The candidate binaries on the N100 are under `/boot/home/native-indexubo-104`:

| Binary | SHA-256 |
|---|---|
| `intel_memory_owner` | `569aebe3880d0fb39fa95c1f5f217d319526117274295a825742f3caa9fff549` |
| `intel_native_execution_bridge` | `60332bc4d49a8f5592b10546e660c9cf5286e401595e51a4a33ec2d0d32cef2a` |
| `libvulkan.so` | `694a001ff59e2e2685efc8055dc8fd63403cc8707f5fb21a59b8282b48812346` |
| `triangle_probe` | `ded9f20bb41b48adc3a5feb033e35bbe3fba19a9bc18fd3a8c3d6f4c3b8e5d0b` |

The installed owner and bridge replaced checkpoint 100 after copies of both
were saved in the same directory as `rollback-*`. The new command profile
adds 907 bytes to the selected runtime source inventory, now approximately
**14,839,534 bytes**. This remains a partial source estimate, not a complete
kernel plus userspace model-token count. The 3–5 MB target remains unmet.

This checkpoint supports a bounded exact fragment program and a single UBO;
it does not establish general SPIR-V shader execution or Vulkan conformance.
