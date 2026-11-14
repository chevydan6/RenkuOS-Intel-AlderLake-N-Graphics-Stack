# Native Vulkan fence after a physical N100 draw

The Vulkan triangle probe now creates an unsignaled fence, confirms
`vkGetFenceStatus` returns `VK_NOT_READY` and a zero-time wait returns
`VK_TIMEOUT`, submits its N100 draw with that fence, then requires signaled
status and successful wait. It verifies the exact 32×32 image hash, resets the
fence, confirms unsignaled status and timeout again, and destroys the fence.

The compiler-backed Vulkan build and its CPU/API probes passed in the Haiku VM.
On the physical N100, the installed owner and Vulkan library remained the
previous validated versions (SHA-256 `c5f72eb814f6e6169004870cd11a02c1b608be9f60b478947e7dfc3e47a0cf6b`
and `098358d81e35782a07b1ef45eb693d855f521d047a647ba87cea53c269c27964`).
Only the triangle application probe changed; its SHA-256 was
`fe8626b028ab4220263378452c6132227526adb83b70809f5318dee36a71f315`.

Four shapes and four green values passed four repeats each: **64 successful
fence-backed draws**, exact image hashes, and 64 owner version-7 completion
traces with matching submission and observed sequences, matching checked and
matched pixel counts, and `fault=0`. Sequence numbers were 237–300. The trace
is [hardware-repeat-64.log](hardware-repeat-64.log), SHA-256
`fc8b966e2f388fb9ba2e3b582716dc921c5519d4b129afacd682ce5082e50317`.

`vkQueueSubmit` is currently synchronous: it signals the host fence only after
the owner reports GPU completion and the output image is copied into its bound
resource. This test proves the hardware draw and fence ordering for that path;
it does not demonstrate asynchronous queue execution or general Vulkan
synchronization primitives.
