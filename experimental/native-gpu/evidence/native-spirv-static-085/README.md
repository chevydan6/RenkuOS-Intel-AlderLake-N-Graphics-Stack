# Static SPIRV-Tools validator link experiment, checkpoint 085

The native compiler calls `spvContextCreate`, `spvValidateBinary`,
`spvDiagnosticDestroy`, and `spvContextDestroy` from the pinned SPIRV-Tools
shared library. This host experiment tested whether linking those calls from
an archive could materially reduce the source closure while retaining the
existing validator.

The archive contains the 80 position-independent object files from the
`SPIRV-Tools-shared` target used in
[`native-spirv-tools-closure-081`](../native-spirv-tools-closure-081/README.md).
A small C probe called the same four functions with a malformed Vulkan 1.0
SPIR-V module. It linked and returned the expected validation failure. The
linker map selected **73 of 80** archive members.

| Source inventory | Shared target | Static validator selection | Difference |
| --- | ---: | ---: | ---: |
| Compiled implementation files | 80 | 73 | 7 |
| Canonical SPIRV-Tools and SPIRV-Headers source files, including headers | 137 | 125 | 12 |
| Canonical source bytes by path | 2,731,422 | 2,690,992 | 40,430 |

The 12 omitted files are seven implementation files and their five private
headers. They contain no Vulkan validator logic needed by this test. This
is a **host source-selection result**, not a Haiku binary or hardware test.
It does not establish a 40,430-byte reduction in the full deduplicated stack,
because some content may already occur elsewhere. Building a Haiku static
archive and replacing the native compiler's shared dependency would add
packaging work for at most this small source-closure gain. The current
validated shared-library path is retained.

The source inventory was computed from `compile_commands.json`, the selected
archive members in the linker map, and each selected object's `.d` dependency
file. The pinned source and host build paths are recorded in checkpoint 081.
No kernel or Vulkan runtime code changed, and no N100 hardware behavior is
claimed for this experiment.
