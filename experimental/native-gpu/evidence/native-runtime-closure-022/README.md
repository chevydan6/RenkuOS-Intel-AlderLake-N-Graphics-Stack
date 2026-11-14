# Native Intel compiler runtime link/include closure (Mesa 26.3.0)

This snapshot measures the compiler runtime used by the current native Vulkan
pipeline, with `intel_spirv_runtime.c` SHA-256
`eac610c58d2ecd783f421889dca851769a12234a88368eed9ae0b9a3b30d71e4`.
The runtime was compiled into the pinned Mesa 26.3.0 build and relinked with
SPIRV-Tools. The selected Mesa archive objects came from the existing VM
build; the relinked library is not byte-identical to the staged production
library. Its compiler pipeline probe passed, accepting bounded RGB machine
code and rejecting changed instructions and invalid input.

`summary.json`, `objects.txt`, and `files.txt` are the generated measurement.
The link map selected 352 objects. Their compile dependencies include 533
Mesa files totaling 14,025,279 bytes, two project runtime files totaling
9,048 bytes, 49 generated files totaling 62,392,205 bytes, and 294 external
headers totaling 5,038,335 bytes. File bytes count complete source/header
files, not just used lines. The generated total includes build outputs and
must not be treated as canonical source size.

This is a **partial link/include closure**, not the complete graphics stack or
a minimum port. It excludes the SPIRV-Tools implementation source, native
Vulkan and kernel driver source, build-only generators, tests, and licenses.
It is not a model-token measurement. In particular, it cannot establish a
complete 3–5 MB source target.

The VM used for this measurement panicked on unmounting a populated temporary
RAM filesystem (`fs_unmount`, `vnodes.IsEmpty()` assertion). After a VM reset,
both BFS volumes passed `checkfs --check-only`; staged Vulkan probe files were
refreshed from canonical source and rebuilt. The compiler pipeline, Vulkan
API, fence, and command-reset probes passed. The physical N100 and its known
good RGB rendering build were unaffected. The RAM filesystem was left mounted
after the second measurement to avoid repeating that VM unmount path.
