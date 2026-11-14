# Native Vulkan host-fence checkpoint 009

Date: 2026-11-15 (Asia/Tokyo). Branch: `codex/i915-native-adaptation`.

The native Vulkan ABI now implements `vkCreateFence`, `vkDestroyFence`,
`vkGetFenceStatus`, `vkResetFences`, and `vkWaitForFences`. A successful
`vkQueueSubmit` signals its fence after the synchronous owner draw and image
copy complete. A zero-batch submission can also signal a fence after all
earlier work on this synchronous queue. Failed submissions leave it unsignaled.
The implementation stores host-side fence state in the device and uses Haiku's
`system_time` and `snooze` for bounded waits. It does not add a kernel fence
ABI or claim asynchronous GPU completion.

The VM fence probe initially exited 1 because `vkGetDeviceProcAddr` had no
fence entry points. After implementation, the compiler-backed VM build ran
the existing compiler and API probes and the new fence probe:

```text
compiler pipeline probe: matched hardware-tested fragment; rejected altered input and short buffer
Vulkan API probe: rejected malformed shader module; compiled accepted pipeline; rejected altered pipeline
Vulkan fence probe: timed wait; empty submit signal; reset; signaled creation; wait-any/all
```

The default noncompiler `libvulkan.so` also compiled with `-Werror`, and its
fence probe passed in the VM. The VM binary hashes are:

| Binary | SHA-256 |
|---|---|
| Compiler-backed `libvulkan.so` | `395d08d0650a97cdd60e3ee83194ba0e44591b7fedb3f7fc000a33a92b613531` |
| Default `libvulkan.so` | `74e68c5ea16932bbf9a8c8efe3cdd1e4765c0c62f04581ab1b75e964c932f1d7` |
| Compiler-backed `fence_probe` | `b258863a737d2a6a851837c881cd3e30f883b7ef95cfd8b7bc1a23c320018596` |

No new binaries were installed on the physical N100. Its original GPU
triangle bundle remains the hardware regression baseline. The read-only BFS
boot volume is recorded in [checkpoint 008](../native-ramfs-unmount-008/README.md).
The zero-batch fence behavior follows the
[Vulkan queue submission specification](https://docs.vulkan.org/refpages/latest/refpages/source/vkQueueSubmit.html).
