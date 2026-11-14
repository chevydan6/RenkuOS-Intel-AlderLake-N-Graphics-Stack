# Native Vulkan N100 synchronous submission timing

The opt-in `RENKU_VK_TRACE=1` submission event now records three wall-clock
durations in microseconds: `render_ioctl_us` for the owner render ioctl,
`write_ioctl_us` for the native resource write ioctl, and `submit_us` for the
whole `vkQueueSubmit` call through image publication. The two ioctl timings
are sampled immediately around their calls. When tracing is disabled, these
clock reads are skipped. A failed render or resource write still emits the
event, with `write_ioctl=-1` if the write was skipped.

The compiler-backed library built and passed its CPU/API probes in the Haiku
VM. Its SHA-256 was
`0b7cea53bcb54f1e2129f25a8a6eb05da7e696630321decc59b5b4c349e3167d`.
The installed N100 owner stayed at SHA-256
`c5f72eb814f6e6169004870cd11a02c1b608be9f60b478947e7dfc3e47a0cf6b`.

The physical N100 was already bootstrapped. Each sample launched the fenced
triangle application as a separate process, submitted one 32×32 draw, and
verified its full image hash. Original, 0.5, 0.75, and 0.9 green shaders each
ran across four shapes for eight passes: **128 exact draws**, owner sequences
302–429, matching observed sequences and pixel counts, and no reported faults.
The raw [hardware-timing-128.log](hardware-timing-128.log) has SHA-256
`027d491bcc99753bc03dd7b95b08140cb45efd57ebaa55dc2cbd96bc3e30f9a3`.

| Host-observed duration | Minimum | Median | 95th percentile | Maximum |
|---|---:|---:|---:|---:|
| Owner render ioctl | 5,214 µs | 5,949 µs | 5,985 µs | 6,094 µs |
| Resource write ioctl | 4 µs | 9 µs | 10 µs | 12 µs |
| Whole `vkQueueSubmit` | 5,223 µs | 5,960 µs | 5,998 µs | 6,105 µs |

Percentiles use the sorted 128 samples and index `round(0.95 * 127)`.
The render ioctl includes kernel setup, GPU execution, completion polling,
and validation. These numbers do not isolate GPU execution time or include
shader compilation, process startup, or presentation. No identical Linux
baseline has been measured yet.
