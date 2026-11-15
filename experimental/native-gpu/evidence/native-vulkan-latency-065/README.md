# N100 Vulkan submission latency baseline (065)

The physical Intel N100 ran the committed 064 Vulkan bundle for three runs of
64 draws in each of four configurations: 32×32 and 64×64 images, each with
direct image-BO rendering or the readback-and-write fallback. All **768 draws**
completed with unique matching owner sequence IDs, no reported fault or ioctl
error, and the expected final pixel hash. The [raw traces](32-1-0.log),
[summary](summary.json), [runner](run.py), and [offline verifier](verify.py)
preserve the results. Run `python3 verify.py` to recompute all measurements
from the saved traces. [Physical hashes](physical-hashes.log) identify the
Vulkan library, probe, and Mesa-derived compiler used.

| Image | Path | Submit median | Submit p95 | Render ioctl median | Extra write median |
|---|---|---:|---:|---:|---:|
| 32×32 | direct | 5,364 µs | 5,503 µs | 5,362.5 µs | 0 µs |
| 32×32 | fallback | 5,385 µs | 5,521 µs | 5,376.5 µs | 7 µs |
| 64×64 | direct | 5,433.5 µs | 5,547 µs | 5,428 µs | 0 µs |
| 64×64 | fallback | 5,398.5 µs | 5,569 µs | 5,380 µs | 13 µs |

`submit_us` is measured inside `vkQueueSubmit` for each draw, including the
blocking owner render ioctl and any fallback resource writes. It excludes
application setup, shader compilation, allocation, and final host readback.
`render_ioctl_us` includes owner validation, GPU command submission,
completion wait, and pixel inspection; it is **not GPU execution time**. The
direct path removes the extra write ioctl, but the roughly 5.3–5.4 ms render
ioctl dominates both paths. The next performance investigation needs timings
inside the native owner to separate command preparation, GPU wait, and pixel
verification before changing the execution architecture.

The current owner also poisons the image before each draw and checks one fixed
expected image. That behavior blocks general Vulkan multi-draw render-pass
semantics; extending command recording alone would overwrite earlier draws.
No driver or Vulkan runtime code changed for this measurement. The selected
runtime source closure therefore remains **12,633,124 bytes** from checkpoint
064; the scripts and traces here are test evidence outside that closure.
