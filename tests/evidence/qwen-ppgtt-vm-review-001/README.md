# Independent review of Qwen's native PPGTT VM tests

Disposition: accepted; see `coordination/ASTRA_QWEN_VM_REVIEW.md`.

Original and independent rebuilt binaries passed2150 encoding/range and69 tree
cases each. All six build/run exits are0; original/rebuilt binaries match exactly.
The two headers/two sources match the current project. Kernel/SDK hashes match
the A34 baseline. Commands, logs, exit codes and artifact hashes are retained here.

These are native guest userspace software tests. No GPU access, native kernel
fixture, acceleration or Vulkan claim. Original Qwen report lacked saved logs;
the independent results corroborate it without claiming its original history.
