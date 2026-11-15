# Remove redundant RCS draw visibility flush (068)

For a draw, `NativeRenderBuild` prepares the ring and context, then
`RcsEngine::Publish` flushes CPU writes before the GPU doorbell. The previous
owner also performed a full-cache `NativeVisible` immediately after the build.
Between those two flushes, the draw path only validates/rearms state and checks
sealed bindings. This change removes that earlier flush for draws. Bootstrap
keeps its previous ordering, and the `Publish` flush and post-completion flush
remain. The public owner ioctl ABI and Vulkan library are unchanged.

The [VM timing build](vm-timing-build.log) and [normal build](vm-normal-build.log)
passed the owner smoke suite. The isolated timing owner ran **32 baseline-style
draws** and then **198 rendering regression draws** on the physical N100. The
[timing trace](owner-timings.log) shows two `NativeVisible` calls per draw rather
than three, with the same expected pixel hashes and completed reports. The
[analyzer](analyze.py) recomputes the [summary](summary.json) from the matched
owner and Vulkan traces. The four copied regression runners preserve their
own output logs in this directory; [the candidate verifier](verify-candidate.py)
checks their 198 distinct draw sequence IDs and pixel hashes offline.

The optimized normal owner, with logging disabled, is now installed at SHA-256
`e37a268c5ce9c7d0109e569c6c2a6f54bdadd7c7891f762c7feae8582784b219`.
The previous known-good owner remains at
`/boot/home/native-render-flush-068/rollback/intel_memory_owner` with SHA-256
`f4a4bce6b7fd5b45ef81867d3648b77a6690ee72fc2104aa2785b9defe557528`.
[Physical hashes](physical-hashes.log) also identify the unchanged Vulkan
library and compiler. The normal build passed **768 repeated benchmark draws**
([offline verifier](normal-latency/verify.py)) plus **206 shader, resource,
fallback, and semaphore regression draws**
([offline verifier](normal-regression/verify.py)).

| Blocking submit median | 065 previous owner | 068 optimized owner | Change |
|---|---:|---:|---:|
| 32×32 direct | 5,364 µs | 4,882.5 µs | −9.0% |
| 32×32 fallback | 5,385 µs | 4,858 µs | −9.8% |
| 64×64 direct | 5,433.5 µs | 4,877 µs | −10.2% |
| 64×64 fallback | 5,398.5 µs | 4,795 µs | −11.2% |

Each row contains 192 verified draws (three runs of 64). These are measured
blocking submission times, not pure GPU execution times. The 64×64 direct p95
was 5,625 µs versus 5,547 µs in checkpoint 065, so the tail did not improve
uniformly. The hardware sequence and expected pixel checks passed in both
builds. The normal source [closure](stack-manifest.json) is **12,635,680 unique
selected canonical bytes**, including optional diagnostic code; model tokens
and some external implementation dependencies remain unmeasured.
