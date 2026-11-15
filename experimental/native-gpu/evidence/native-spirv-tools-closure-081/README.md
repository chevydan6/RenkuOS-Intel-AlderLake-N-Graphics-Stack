# SPIRV-Tools source closure, checkpoint 081

The native Intel compiler loads `libSPIRV-Tools-shared.so` to validate Vulkan
SPIR-V modules. Checkpoint 080's 12,502,676-byte source inventory did not
include that library's implementation. This checkpoint measures its selected
source from the package's pinned upstream version.

## Provenance and method

The [HaikuPorts SPIRV-Tools recipe](https://github.com/haikuports/haikuports/blob/master/dev-util/spirv-tools/spirv_tools-1.4.350.0.recipe)
uses Khronos `vulkan-sdk-1.4.350.0`, its Haiku patch set, and the matching
[SPIRV-Headers recipe](https://github.com/haikuports/haikuports/blob/master/dev-util/spirv-headers/spirv_headers-1.4.350.0.recipe).
The downloaded source archives matched both recipe SHA-256 values; see
`source-hashes.log`. Local copies of both recipes and the byte-identical
compressed patch set preserve the exact inputs used here.

The pinned source was configured with the recipe's shared-library settings,
the Haiku patch set, and `CMAKE_EXPORT_COMPILE_COMMANDS=ON`. On the host,
`SPIRV-Tools-shared` built successfully. `measure-spirv-tools-closure.py`
reads only that target's 80 compile records and their dependency files,
records SHA-256 for each input, and deduplicates canonical source by content
against checkpoint 080's stack manifest. `host-build.log` and `closure.json`
preserve the build and inventory. The host build does not replace the native
Haiku library used by the N100.

| Selected input category | Files | Bytes by path |
| --- | ---: | ---: |
| SPIRV-Tools canonical source | 132 | 2,406,271 |
| SPIRV-Headers canonical source | 5 | 325,151 |
| Generated headers/tables | 6 | 827,057 |
| Host SDK headers | 319 | 4,809,619 |

The 137 project source files contain **2,731,422 unique bytes**. Of those,
2,719,492 bytes are new relative to the prior stack. The resulting
**host-derived selected runtime source inventory is 15,222,168 unique bytes**.
Generated files and host SDK headers are separate in `closure.json`; build-only
generators and other kernel/build inputs remain unmeasured. The development
model's tokenizer is unavailable, so token count remains unmeasured.

## Limit of this measurement

The exact HaikuPorts package build dependency files were unavailable. An
attempt to install CMake and reproduce its target on the Haiku VM stopped when
the 650 MiB VM volume had only about 20.6 MiB free and package download ran
out of space. The temporary source archives copied there were removed. The
host build uses the package's source tag, patch set, and target definition,
but its included platform headers can differ from Haiku's. Treat 15,222,168
bytes as a measured host reproduction of the selected target, not an exact
source closure of the installed Haiku package or the whole graphics stack.

This closes one missing dependency in the size inventory. The 3–5 MB source
target remains unmet, and the runtime behavior is unchanged.
