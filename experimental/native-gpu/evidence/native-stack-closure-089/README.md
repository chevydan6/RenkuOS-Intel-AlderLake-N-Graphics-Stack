# Current native Vulkan selected-source inventory, checkpoint 089

This rebuilds the source inventory after the physical N100 version-21
preserving-batch checkpoint. The installed owner, bridge, and Vulkan library
were built from the corresponding Haiku VM bundles. Their current compile
dependency files are saved under `owner-deps/` and `vulkan-deps/`.

`measure-native-closure.py` and `vulkan/measure-closure.py` hashed each
referenced file on the Haiku VM. `measure-stack-closure.py` combined those
inputs with the pinned Mesa 26.3.0 compiler file list from checkpoint 080.
`measure-spirv-tools-closure.py` then merged the pinned SPIRV-Tools and
SPIRV-Headers implementation source from checkpoint 081's host reproduction.
All manifests retain paths, byte counts, and SHA-256 values.

| Selected runtime source accounting | Bytes |
| --- | ---: |
| Compiler, native owner, and Vulkan inputs, unique by content | 12,522,402 |
| Added SPIRV-Tools and SPIRV-Headers source, unique by content | 2,719,492 |
| **Combined selected runtime source** | **15,241,894** |

The stack manifest lists 497 selected Mesa source/include files totaling
11,890,881 bytes by path. The pinned SPIRV-Tools target has 137 project source
and header inputs totaling 2,731,422 bytes by path. Khronos headers in the
Vulkan bundle total 1,459,702 bytes by path; the large `vulkan_core.h` copy is
also present in Mesa, so the unique-content total does not count it twice.
These component byte counts overlap and must not be summed as a budget.

Checkpoint 088's 15,230,927-byte figure was explicitly a delta estimate.
The full inventory is **10,967 bytes higher**. Comparing unique source hashes
with checkpoint 080 shows 157,132 bytes of added or changed source and
137,406 bytes of removed/replaced source, a 19,726-byte net change before
SPIRV-Tools. The delta estimate did not track every newly selected file.

This remains a **selected runtime source inventory**, not the complete
operating-system or build dependency closure. It excludes generated code,
build-only generators, probes, SDK headers, broader kernel sources, and
unselected paths. SPIRV-Tools selection was reproduced on the host using the
pinned package source rather than the exact Haiku package build. The actual
development-model tokenizer is unavailable here, so model tokens are not
reported; raw source bytes are not a token estimate. The 3–5 MB source target
is unmet.
