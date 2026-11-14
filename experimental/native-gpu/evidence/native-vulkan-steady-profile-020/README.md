# N100 steady-state command reuse and kernel profile

The triangle application gained a bounded repeat argument (`1..64`). It
records one command buffer, submits it repeatedly after each synchronous
completion, resets and reuses one fence, and verifies the final exact 32×32
image. This is a steady-state regression and profiling path; it does not add
asynchronous queue execution.

The compiler-backed Vulkan library and its VM probes passed unchanged. The
new triangle application SHA-256 was
`8cf40b286ea2b0b25e4b03c02b77b5eabc67713cac98d8dfa273985e02b7be65`.
On the physical N100, one `green90` shape-0 application run submitted the
same command buffer and used the same vertex, shader, and image BOs 64 times.
All 64 owner version-7 submissions completed, with exact pixel hashes,
matching sequence and pixel counts, and no reported faults. The raw
[repeat-command-64.log](repeat-command-64.log) has SHA-256
`a9d3ce5dbe7210ada6416e33fb44215fda2976c96104fc657f20c7544989327b`.

| Host-observed duration, 64 traced submits | Minimum | Median | 95th percentile | Maximum |
|---|---:|---:|---:|---:|
| Owner render ioctl | 4,957 µs | 5,176 µs | 5,214 µs | 5,277 µs |
| Resource write ioctl | 4 µs | 6 µs | 6 µs | 10 µs |
| Whole `vkQueueSubmit` | 4,966 µs | 5,184 µs | 5,222 µs | 5,291 µs |

Percentiles use the sorted samples and index `round(0.95 * 63)`. The prior
separate-process sample had a 5,960 µs median submit; the two workloads differ
in resource and command reuse, so this comparison does not isolate one cause.

Haiku's `profile -k -i 100 -S` sampled three more 64-draw application runs.
Among captured ticks, `ECAMPCIController::ReadConfig` had 42.07%, 39.59%, and
43.67%; `GroupRead` had 32.01%, 34.28%, and 22.92%; and
`NativeDmaStillDirect` had 8.38%, 8.60%, and 10.31%. The three raw captures
are [run 1](profile-repeat-64.txt), [run 2](profile-repeat-64-2.txt), and
[run 3](profile-repeat-64-3.txt). The profiler missed 888/3512, 897/3514,
and 854/3027 expected ticks respectively, so percentages are indicative.

The owner calls PCI/power, forcewake ACK, and DMA admission checks repeatedly
inside the render operation. These profiles identify where to investigate
overhead. They do not justify removing any ownership or power check without a
separate safety argument and hardware regression. GPU-only execution time
remains unmeasured.
