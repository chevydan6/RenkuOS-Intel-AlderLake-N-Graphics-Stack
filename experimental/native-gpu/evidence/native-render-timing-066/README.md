# N100 native RCS owner timing experiment (066)

Checkpoint 065 measured roughly 5.3–5.4 ms per blocking Vulkan draw inside
the native render ioctl. This experiment split the owner work into stages with
an optional `NATIVE_RENDER_PROFILE_TIMING=1` build flag. Normal builds omit the
timing calls and log line. The public [render request/report ABI](../../RenderReport.h)
did not change. The isolated instrumented owner passed the [Haiku VM smoke
suite](vm-build.log), was loaded on the physical N100 for this sample, and was
then replaced by the previous owner with hash
`f4a4bce6b7fd5b45ef81867d3648b77a6690ee72fc2104aa2785b9defe557528`.
The [restored-owner smoke](restored-owner-smoke.log) passed eight 32×32/64×64
draws on direct and fallback paths after the second reboot.

The physical N100 completed 16 direct-target draws at 32×32 and 16 at 64×64.
All 32 had matching owner sequence IDs, expected pixel hashes, completed GPU
reports, and no fault. [Owner timings](owner-timings.log) and [Vulkan traces for
32×32](triangle32.log) and [64×64](triangle64.log) are joined by sequence ID
in [the analyzer](analyze.py). Recompute the [summary](summary.json) with
`python3 analyze.py | diff - summary.json`.

| Stage, median µs | 32×32 | 64×64 |
|---|---:|---:|
| Admission to forcewake | 10 | 10 |
| Forcewake inspection | 333 | 335 |
| Group setup before draw work | 458.5 | 462.5 |
| Draw work through command publication | 2,405 | 2,446.5 |
| CSB completion wait | 1,117 | 1,122.5 |
| Post-completion verification | 726 | 768 |
| Group cleanup | 199.5 | 199.5 |
| Owner total before log | 5,265 | 5,340 |
| Render ioctl observed by Vulkan | 6,033 | 6,126 |

The 2.4 ms before publication is the largest measured owner stage. It includes
`NativeRenderProbe`, validated image and batch construction, visibility work,
and RCS rearm; these operations have not yet been timed separately. The CSB
wait measures completion observation, including polling, and is **not a pure
GPU execution time**. The difference between owner total and userspace ioctl
time includes the diagnostic `dprintf` and bridge/kernel return path; it must
not be attributed solely to the bridge. Logging raises this candidate's
userspace ioctl median by about 0.7 ms relative to the normal 065 baseline.

The isolated candidate's [owner manifest](owner-manifest.json) and [combined
manifest](stack-manifest.json) measure **12,635,067 selected canonical source
bytes**, 1,943 bytes above checkpoint 064. This counts the optional diagnostic
source. The installed normal owner, compiler, and Vulkan binary are unchanged;
its last measured closure remains 12,633,124 bytes. Model tokens and the full
external dependency closure remain unmeasured.
