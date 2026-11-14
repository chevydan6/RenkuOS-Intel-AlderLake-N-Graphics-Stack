# Native Vulkan individual command reset checkpoint 011

Date: 2026-11-15 (Asia/Tokyo). Branch: `codex/i915-native-adaptation`.

`vkCreateCommandPool` now accepts the Vulkan 1.0
`VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`. Such a pool supports
`vkResetCommandBuffer`, and beginning an executable buffer implicitly resets
it before recording. Pools without that flag still use `vkResetCommandPool`.
The shared reset helper clears recorded object references and lifecycle state
without discarding the buffer allocation.

The command reset probe first exited 10 because the individual-reset entry
point was missing. With the implementation, it checked that a pool without
the reset flag rejects individual reset, then explicitly reset a recording
buffer and implicitly reset an executable buffer. The compiler-backed VM
build ran all four probes successfully:

```text
compiler pipeline probe: matched hardware-tested fragment; rejected altered input and short buffer
Vulkan API probe: rejected malformed shader module; compiled accepted pipeline; rejected altered pipeline
Vulkan fence probe: timed wait; empty submit signal; reset; signaled creation; wait-any/all
Vulkan command reset: pool, explicit buffer, and implicit begin reuse
```

The compiler-backed VM `libvulkan.so` SHA-256 is
`e8c474cabc06bef34c4b38e2a36944eebb01fac9f5968baadd3662a5fb10cbe9`.
The default noncompiler build passed the same command reset probe, and its
`libvulkan.so` SHA-256 is
`9ad12788e53b1820301d3ef2444af0dea8a293f12b499a2568c4d75a05c1919b`.
No physical N100 binary was replaced or tested in this increment.

The behavior follows the
[Vulkan command-buffer lifecycle](https://docs.vulkan.org/spec/latest/chapters/cmdbuffers.html).
