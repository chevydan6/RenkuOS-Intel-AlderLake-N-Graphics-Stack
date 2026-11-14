<!-- SPDX-License-Identifier: MIT -->
# Native Vulkan triangle in a Haiku window

The compiler-backed native Vulkan library rendered a 32×32 triangle on the
physical Intel N100. The application verified the exact output hash
`28616d2d` and representative pixels, copied the completed BGRA image to a
Haiku `BBitmap`, submitted a draw through `BWindow`/`BView`, and exited normally
after its three-second close timer. The hardware trace in [physical.log](physical.log)
records successful submission 49, zero GPU fault, and 777 matching checked
pixels. Submission took 5,665 microseconds in that run; this is one sample,
not a throughput benchmark.

Build on RenkuOS with `vulkan/build-vulkan-window.sh`, pointing
`RENKU_VULKAN_HEADERS` to the extracted Mesa 26.3.0 Khronos headers and
`RENKU_VULKAN_LIB_DIR` to the compiler-backed native library. On the N100,
run the built `triangle_window` with its library and compiler dependency
directories in `LIBRARY_PATH`. `--close-after-ms 3000` makes the check bounded.

This demonstrates CPU-visible Vulkan image readback and a native Haiku window
draw path. The hardware run was verified by log; no screen capture was taken
for this checkpoint. It does not implement Vulkan WSI, swapchains, zero-copy
display, or vblank synchronization. No kernel or installed graphics component
changed.
