# Mesa 26.3.0 shaders on physical N100, 2026-11-14

The pinned Mesa 26.3.0 Intel compiler produced a 128-byte VS and 208-byte FS
from the deterministic NIR harness. The existing command-image emitter was
rebuilt against Mesa 26.3.0 packet headers; the resulting 323-dword batch
passed the independent Gen12 XML decoder and CPU smoke. The regenerated
header SHA-256 `170edca1f69fbd33dea3ec506b9760b94700223fb200f95d5f21664c6f46ea36`
matches the exact header copied into the isolated N100 build bundle at
`/boot/home/native-rcs-shader-263-013`. [Source hashes](source.sha256) and
[native build log](native-build.log) record inputs and build checks.

The candidate owner and bridge were installed after saving the previous
known-good v5 binaries as `known-good-v5-owner` and `known-good-v5-bridge` in
the bundle. [Installed hashes](installed-hashes.txt) are owner
`e06a8125908c9bb61e87cc5c4dba7552485a62235dc2974a2dd14c0b3c658e9e`
and bridge `5a0899fa8d449ffbe14bb3dd19c21af5a20d02e47de0d9ee6c3c5e08696f3711`.
No RenkuOS base-kernel source or Linux graphics infrastructure changed.

After reboot, read-only probe, explicit bootstrap, and one fixed triangle
draw passed. The first draw had 654/654 matching pixels and the original
`28616d2d` image hash. Subsequent regressions used four fixed shapes per
round, checked expected pixel counts and hashes, and required gap-free
completion sequences:

| Physical regression | Result | Final sequence |
| --- | --- | ---: |
| [64 public-batch v5 draws](v5-64/metadata.json) | PASS | 66 |
| [64 fixed v1 draws](v1-64/metadata.json) | PASS | 130 |
| [64 public-vertex v4 draws](v4-64/metadata.json) | PASS | 194 |
| [64 separate-page v3 draws](v3-64-corrected/metadata.json) | PASS | 258 |
| [64 user-vertex v2 draws](v2-64-corrected/metadata.json) | PASS | 322 |
| [Four final v5 draws](v5-after-controls/metadata.json) | PASS | 326 |

One first attempt at the v3 runner exited with a usage message because it
was pointed at the v5-only client. Its [raw output](v3-64/draws.raw.log)
shows no GPU request; the corrected v3 run passed. Four malformed public
batches were rejected before submission, resource create/read/write/destroy
passed, and the final read-only probe passed; see
[final checks](final-negative-resource-probe.log).

The physical test lock was released. The Govee outlet remained on and was
not cycled. The Mesa 26.3.0 image is now the installed RCS shader baseline.
This validates only the bounded, exact command template. Native userspace
SPIR-V compilation, general Vulkan queues, asynchronous fences, VM binding,
and presentation remain unimplemented.
