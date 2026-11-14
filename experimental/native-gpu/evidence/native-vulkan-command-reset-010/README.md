# Native Vulkan command-pool reset checkpoint 010

Date: 2026-11-15 (Asia/Tokyo). Branch: `codex/i915-native-adaptation`.

`vkResetCommandPool` now returns all command buffers in a pool to the initial
state. The VM probe records a complete draw through the public Vulkan commands,
resets the pool, and begins recording the same buffer again. It also checks
reset while the buffer is recording. The existing implementation had no
`vkResetCommandPool` entry point; the probe exited 1 before the change.

The compiler-backed VM build and all four probes passed after the change:

```text
compiler pipeline probe: matched hardware-tested fragment; rejected altered input and short buffer
Vulkan API probe: rejected malformed shader module; compiled accepted pipeline; rejected altered pipeline
Vulkan fence probe: timed wait; empty submit signal; reset; signaled creation; wait-any/all
Vulkan command-pool reset: reused recording and executable buffers
```

The rebuilt VM `libvulkan.so` SHA-256 is
`91d0cffb1c0b71ac6ad5606f654216450ecfdc49948a574a520dd13d61aa5303`.
The default noncompiler variant also compiled with `-Werror`, passed the
same command reset probe, and hashed
`164332573dc4dd5235625c64e37fead9612df315189941962a049764f6e5d0d6`.
This is a host-side lifecycle change. It has not been installed on the N100
because its BFS boot volume remains read-only. Individual command-buffer reset,
semaphores, and general command-buffer workloads remain open.

The intended lifecycle follows the
[Vulkan command-pool reset specification](https://docs.vulkan.org/refpages/latest/refpages/source/vkResetCommandPool.html).
