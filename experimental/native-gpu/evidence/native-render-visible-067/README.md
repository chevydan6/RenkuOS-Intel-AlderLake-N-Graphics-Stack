# N100 full-cache visibility cost (067)

The optional owner timing build now counts the elapsed time inside
`NativeVisible`, which executes `mfence; wbinvd; mfence`. This candidate kept
the original draw behavior. It passed the [VM owner smoke suite](vm-build.log)
and then **32 N100 draws** at 32×32 and 64×64, all with matching pixels and
hardware completion reports. [Raw owner timings](owner-timings.log),
[Vulkan traces](triangle32.log), the [analyzer](analyze.py), and the
[summary](summary.json) preserve the result. Run
`python3 analyze.py | diff - summary.json` to check the saved evidence.

| Median per draw | 32×32 | 64×64 |
|---|---:|---:|
| Full-cache flush calls | 3 | 3 |
| Combined flush time | 1,499.5 µs | 1,497 µs |
| Work before command publication | 2,731.5 µs | 2,728 µs |
| Owner total before diagnostic log | 6,008 µs | 6,049 µs |

The three flushes occur before the first RCS publication, inside
`RcsEngine::Publish` before GPU submission, and after completion before CPU
inspection. The aggregate is measured; individual call durations were not
recorded. Cross-reboot differences from the 066 timings cannot establish a
performance regression caused by the counters.

The instrumented owner had SHA-256
`8190aa6d856a839a3c0f101d3e15fe7c1d9d8965aa069ccc382bfee40d4c0f8e`.
The known-good owner was restored afterward, and [eight direct/fallback
draws](restored-owner-smoke.log) passed following reboot. The isolated
[source closure](stack-manifest.json) is **12,635,510 selected canonical bytes**.
This includes optional timing code; it is not a complete dependency or
model-token count. Checkpoint 068 uses these measurements to remove one
duplicate pre-publication flush.
