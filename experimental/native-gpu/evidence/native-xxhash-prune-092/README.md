# Narrow native xxHash compiler input, checkpoint 092

The selected Mesa 26.3.0 Intel compiler included `src/util/xxhash.h`
(268,319 bytes) through seven selected objects. Those objects call only the
one-shot XXH32 and XXH64 functions. The native adaptation retains those
algorithms in a 6,512-byte BSD-licensed header and leaves XXH3 and streaming
APIs out of this selected compiler build. The linker checks the SHA-256 of
Mesa's original header and `hash_table.c` before applying the override.

Six includers use `"util/xxhash.h"`; `hash_table.c` includes the header from
its own directory. The linker therefore generates one exact copy of the
pinned `hash_table.c` with that include changed to `"util/xxhash.h"`, then
recompiles the seven affected objects with the native include root. The
pinned Mesa tree is unchanged. `native_hash_table.c`, `compiler-files.txt`,
`compiler-objects.txt`, `compiler-report.json`, and `build-final.log.gz`
preserve the isolated Haiku VM build evidence. The selected compiler still
links 345 objects without `libdrm`, and the original xxHash header is absent
from its selected file list.

`check-native-xxhash.py` passed **1,042** comparisons against Mesa's pinned
header across lengths, seeds, alignments, a one-byte buffer, and a null
zero-length input. The final narrow header also passed host ASan and UBSan.
The Haiku VM compiler pipeline and API probes passed with the final library.
A review found and prompted correction of a short-input pointer-bounds issue
before the final build and hardware retest.

The final compiler SHA-256 is
`7b195b9f71116ae92324d06d77b8e63bfe0250bbc7c9e4e385cf21615c70f717`.
`physical-images-and-hashes.log` shows this exact compiler loaded beside the
checkpoint-091 Vulkan library on the physical N100. With that binary, all
**262 previous hardware regression draws** passed. The 32×32 and 64×64
render-pass LOAD images again matched the same-pass images byte for byte;
the `DONT_CARE` second pass remained distinct, and LOAD without a direct
target was rejected before submission. `run-prior-regression.py`,
`run-hardware.py`, their logs, and raw captures preserve the checks. The
installed owner and bridge were unchanged.

Twenty interleaved 100-pair SPIR-V compile samples per library used the
baseline checkpoint-090 compiler and the final candidate on the same N100.
Median CPU elapsed time was 125,724 µs versus 126,300 µs; individual samples
overlapped broadly (baseline 124,496–156,442 µs, candidate
124,741–156,481 µs). This does not establish a speedup or slowdown.
`compiler-benchmark.json` retains every sample and verifies the loaded
library path and compilation checksum.

The original xxHash header's 268,319 bytes are replaced by 6,512 bytes;
the generated `hash_table.c` adds five bytes. `stack-manifest.json` and
`spirv-tools-closure.json` therefore measure **14,810,595 unique selected
runtime source bytes**, **261,802 fewer** than checkpoint 091. This inventory
includes a pinned host reproduction of SPIRV-Tools, but excludes build-only
generators, broader OS source, and the unavailable development-model token
count. The 3–5 MB objective remains unmet.

The native header retains Yann Collet's BSD 2-Clause notice. The generated
hash-table source retains its Intel/Keith Packard/Bart Massey copyright and
license notice. A Mesa update must revalidate the selected hash API and the
two pinned source hashes before this override can be reused.
