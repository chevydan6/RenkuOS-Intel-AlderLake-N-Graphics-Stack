# N100 render-access latency experiment

Date: 2026-11-15 (Asia/Tokyo). Physical target: `renkuos-n100`.

The previous 64-draw kernel profile placed much of the render ioctl's CPU
time in PCI config reads and forcewake group reads. `NativeRenderResetSafe`
rechecked shutdown, PCI/reset state, DMA, and ownership before calling
`NativeRenderRawRead` for the fault register. That read already checks the
same conditions before and after MMIO. A candidate removed the duplicate
outer check; no other owner behavior was changed. The VM owner build and CPU
suite passed (`owner-vm-build.log`). The candidate also passed 72 physical
draws across 21 existing Vulkan cases (`physical-matrix.log`).

The same direct-linked Vulkan binary submitted one 64×64 FragCoord triangle
64 times per process, twice per owner installation. The N100 rebooted between
owner switches. `verify.py` validates all four logs: 128 successful GPU
submissions each, `fault=0`, matching completion sequences, 3,476/3,476
checked pixels, and the exact `7b2f62f5` image hash.

| Owner run order | Owner | Median render ioctl |
|---|---|---:|
| A1 | Previous | 5,992 µs |
| B1 | Candidate | 5,721 µs |
| A2 | Previous | 5,515 µs |
| B2 | Candidate | 5,108.5 µs |

Latency declined across *both* owners in run order. The A/B/A/B series does
not isolate a benefit from removing the duplicate check. The source edit was
reverted. `owner-hashes.log` confirms the previous owner SHA-256
`5bff63fae0cd6bab5887790d994a3c8235237db41ff6ad24b9ce88b8b68ee02b`
is installed after the final reboot; a 1/128 gradient draw passed afterward.
The candidate binary remains only in the isolated VM and N100 experiment
directories. No performance improvement is claimed or merged.

Recalculate the trace checks and timing table:

```sh
python3 experimental/native-gpu/evidence/native-rcs-access-050/verify.py
```
