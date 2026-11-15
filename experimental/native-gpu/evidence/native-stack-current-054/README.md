# Current N100 native Vulkan source closure

This checkpoint measures the **current Haiku VM builds** after direct GPU
rendering into the Vulkan image BO. It combines the native Intel compiler
048c link/include list with the owner 053 and Vulkan 053 compiler depfiles.
`stack-manifest.json` contains paths, SHA-256 hashes, byte sizes, categories,
and the exact input-manifest hashes. The full file list is reproducible with
`measure-native-closure.py`, `vulkan/measure-closure.py`, and
`measure-stack-closure.py` using the arguments below.

The selected canonical runtime source, counted once by content hash and
excluding SDK files, generated files, and probes, is **14,174,197 bytes**.
Selected Mesa source accounts for **13,623,249 bytes**. The compiler's
49 generated inputs add **62,392,205 bytes** to the compile-input view,
including a 49,546,537-byte generated NIR algebraic pass file. The path
union is 1,040 files and 83,445,578 bytes. These figures are source/input
bytes; they are not binary size, archive size, or model tokens.

The 3–5 MB canonical-source objective remains unmet. This inventory also
omits SPIR-V Tools implementation source, build-only generators, RenkuOS
kernel source outside the owner closure, and model-tokenizer counts. The
development model's tokenizer is unavailable in this workspace; no token
estimate was substituted.

## Accounting correction

Compiler 048c uses the native FragCoord runtime at
`/boot/home/test-work/native-fragcoord-runtime-048/`. The previous closure
classifier silently put its C file and header in `compiler_sdk`, excluding
them from canonical runtime totals. The classifier now recognizes both as
`compiler_project_runtime` and rejects unknown project paths. These files
contain 9,255 bytes; the corrected **unique** runtime total increases by
7,491 bytes because the same 1,764-byte header also appears in the Vulkan
bundle. The earlier 045 checkpoint used a different runtime path and
reported 14,168,555 bytes; this 054 measurement is 5,642 bytes higher after
the accounting correction and subsequent source changes.

The selected Mesa source still includes full GL/GLES and Vulkan headers,
among other shared dependencies. `GL/glext.h` alone is 860,150 bytes; the
generated NIR algebraic C file is far larger. Those are candidates for
measured build-closure experiments, not evidence that deleting them is safe.
Any pruning must preserve the compiler output and physical N100 regressions.

## Reproduce

The VM source bundles and exact depfiles are recorded in this directory.
From the repository root:

```sh
python3 experimental/native-gpu/measure-native-closure.py \
  --deps experimental/native-gpu/evidence/native-stack-current-054/owner-deps \
  --bundle /boot/home/test-work/owner-direct-target-053 \
  --owner-source /boot/home/test-work/i915-mapping-002/source \
  --host renkuos-vm \
  --output experimental/native-gpu/evidence/native-stack-current-054/owner-manifest.json
python3 experimental/native-gpu/vulkan/measure-closure.py \
  --deps experimental/native-gpu/evidence/native-stack-current-054/vulkan-deps \
  --bundle /boot/home/test-work/vulkan-direct-target-053 \
  --host renkuos-vm \
  --output experimental/native-gpu/evidence/native-stack-current-054/vulkan-manifest.json
python3 experimental/native-gpu/measure-stack-closure.py \
  --compiler-files experimental/native-gpu/evidence/native-stack-current-054/compiler-files.txt \
  --owner-manifest experimental/native-gpu/evidence/native-stack-current-054/owner-manifest.json \
  --vulkan-manifest experimental/native-gpu/evidence/native-stack-current-054/vulkan-manifest.json \
  --host renkuos-vm \
  --output experimental/native-gpu/evidence/native-stack-current-054/stack-manifest.json
```
