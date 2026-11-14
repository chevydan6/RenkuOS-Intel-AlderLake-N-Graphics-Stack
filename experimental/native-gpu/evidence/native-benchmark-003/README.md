<!-- SPDX-License-Identifier: MIT -->
# Native tile-copy demonstration and benchmark — 2026-11-14

Base: `43edf98`, branch `codex/i915-native-adaptation`. Physical Intel N100,
8086:46d1 revision0, RenkuOS hrev60072+518. Native `-O2 -Wall -Wextra -Werror`
build and export link passed. The resident owner was replaced and rebooted;
the v2 bridge and thirteen original display/observer components were preserved.

The [native window](native-window.jpg) displays the generated32×32pixel source
tile, actual GPU readback and measured results. Both [source](source.ppm) and
[GPU copy](gpu-copy.ppm) are byte-identical ([hashes](images.sha256)). This is a
BCS copy into private memory, followed by ordinary native BeAPI presentation;
it does not demonstrate GPU rendering or accelerated window composition.

## Results

Median per copy, three batches per size/mode:

| Copied bytes | Rectangle | CPU memcpy (ns) | GPU copy ioctl (µs) | Upload + GPU + readback (µs) |
|---:|---:|---:|---:|---:|
|64|4×4|10.13|2664.40|2678.75|
|256|8×8|18.34|2670.60|2679.95|
|1024|16×16|32.55|2664.95|2673.45|
|4096|32×32|89.45|2681.15|2668.95|

The current prototype provides **no speedup at any tested size**. CPU memcpy is
about30000–263000times faster than the full GPU copy path here. The GPU time is
nearly flat across sizes, showing that fixed software/cache/setup costs dominate
this small range. These measurements do not locate a crossover for larger copies:
the current interface only permits4096-byte buffers. Do not extrapolate these
numbers to intrinsic BCS bandwidth or larger GPU-resident workloads.

On this fresh boot, session open plus two buffer creates took97502µs, and the
first GPU copy took15315µs including first device/request setup. Later size-first
calls took2662–2669µs. Setup is reported separately, not silently amortized into
the steady-state table. Slightly lower roundtrip than ioctl-only time at4096bytes
is ordinary batch variation; these are independently measured totals, not
subtracted phase estimates.

[CSV](benchmark.csv) contains full-precision values; [runtime log](runtime.log)
includes all twelve per-trial results and the final PASS. All484GPU requests
completed and retired in one boot, ending at sequence484 with no failures.
Readback checks every copied word and all untouched destination words for each
size and each batch. The kernel separately verifies every request's data and
ordered completion. The native window was captured, then closed normally to
flush the complete log; the system remained responsive.

## Measurement boundaries

- CPU is compiled `memcpy` between warm user-space arrays. Adaptive batches last
  at least20ms to amortize the microsecond `system_time()` clock. An out-of-line
  function plus memory clobber prevents removing/combining repetitions; the
  [native disassembly](cpu-copy-disassembly.log) confirms actual `rep movsq` data
  movement. The wrapper/function-call overhead remains included. CPU correctness
  is checked after every measured batch.
- GPU ioctl timing wraps20complete copy calls per trial, including user report
  validation. It includes kernel staging of both4096-byte buffers, full-page
  hash/data verification, context/image construction, forcewake acquire/release,
  the prototype's whole-machine cache flushes, ELSQ publication, polling, CSB
  interpretation, retirement and destination copyback. No submission, cache or
  waiting costs are removed. This is intentionally a measurement of the actual
  prototype API, not a claim about the copy engine's execution time alone.
- Roundtrip timing additionally uploads the source and reads back the destination
  through the current fixed4096-byte inline transfer ABI on every repetition.
  Initial buffer allocation/destination initialization and final image/GUI/file
  output are outside steady-state measurements. First calls are separately timed.
- Each row is a packed square with pitch=width×4. Buffers remain distinct, owned
  and kernel-validated. CPU and GPU repeat the same unchanged source data; new
  request sequences ensure real GPU submissions. A clock reading brackets each
  batch. There is no per-phase instrumentation or cache-flush-only estimate.
- These are warm CPU vs real prototype API costs, not matched low-level engines.
  CPU memory bandwidth, CPU time consumed by polling, CPU utilization and power
  are not separately measured. Global cache disruption can also affect other
  software; it is not a general cache-management strategy or optimized driver.

## Actual failure and narrow correction

The first run stopped at request30/stage5 ([log](initial-failure.log)); it
produced no complete timing row. After reboot, diagnostic logging identified
request12 failing bootstrap stage3/error4: `WaitSet/PowerChanged`
([log](diagnostic-failure.log)). Neither run continued after failure. The stale
engine/load fields in that diagnostic belong to the previous request; stage5
precedes publication of the failing request. Both failed boots were recovered
by controlled reboot, preserving retained memory until reset.

The native bootstrap/inspection callback still used full historical PCI snapshot
comparison (`HandshakePciStable`), while native group work already used stable
access/power anchors. A native-only adapter now uses that same established
identity/revision/BAR/command/D0 policy while retaining the original mapping,
lease, phase and display-independent cleanup checks. Full initial admission and
legacy diagnostic behavior remain intact; Intel forcewake hardware sequences are
unchanged. The error code alone does not identify which snapshot field differed;
volatile Status/PME comparisons are a plausible explanation, not a captured
field-level finding. The corrected native build completed all484requests.
The existing PCI anchor smoke covers volatile status acceptance and moved-BAR
rejection. Focused static review found no material issue in timing or this fix.

## Artifacts and current target

Source/build/client: `/boot/home/native-benchmark-owner-003`.
Run in a fresh output directory:

```sh
/boot/home/native-benchmark-owner-003/build/gpu_copy_bench --show
```

Omit `--show` for a headless run. The utility writes `benchmark.csv`, `source.ppm`
and `gpu-copy.ppm` in the working directory. `--show` keeps the application open
until its native window is closed. Benchmark source is
[../../gpu_copy_bench.cpp](../../gpu_copy_bench.cpp); the ordinary native bundle
build now compiles it with `-lbe`. This is the observed VT-d-disabled DMA path;
no general IOMMU support is added.

[Installed owner/bridge](installed.sha256), [native build](build.log),
[source hashes](payload.sha256), [source check](source-check.log),
[installed check](installed-after.log), [benchmark binary](benchmark-binary.sha256),
[unchanged components](unchanged-after.log), [kernel](kernel.log) and
[app_server PID127](app-server.log) are retained. Current owner SHA256:
`dbb36045d998c2440f7c32bd82572946d3ca014bb6a852004c87d614251f841d`.
Bridge remains `30d6283481973cec65cbc58be7922c09df3fe98f9a3fd80d4166e82f89b808c8`.
Physical lease `native-benchmark-20261114-001` was released after postchecks.
HDMI build057 and NanoKVM boot media remain unchanged. Requests retired normally;
another copy can run without reboot. Resident-code replacement still needs reboot.

Full sources, objects, binaries and all three runs remain in
`/home/dan/projects/native-benchmark-evidence-003.tar.gz` and target
`/boot/home/native-benchmark-evidence-003.tar.gz`, SHA256
`6bcabd88922db046606704551d089ca449c7a24d44aff3c7a42021379c51fce6`.
Native wall time is incorrect; hashes identify the actual build.
