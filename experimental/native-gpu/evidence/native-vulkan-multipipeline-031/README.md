# Native Vulkan multi-pipeline creation on the N100

`vkCreateGraphicsPipelines` now accepts more than one bounded graphics pipeline
per call. It validates every create info before allocating owner resources and
clears all returned handles if a later creation fails. The existing exact
shader and fixed-state admission rules remain in force. No owner or kernel
binary changed in this checkpoint.

The VM API probe first rejected the two-pipeline request against the previous
library (exit 14), then passed with the new implementation. It also verifies
that an invalid second pipeline is rejected before the first allocation.
The full VM compiler/API/fence/reset build passed (`vm-build.log`).
The original compiler-free library variant also compiled on the VM with
`-Wall -Wextra -Werror` (SHA-256
`eb646517e4ca8c213bcd07e71bfd72a566590f6c2302c3d251958ad752bb4eec`);
that VM compile is not a physical test of the compiler-free path.

The physical probe was staged beside the previous library at
`/boot/home/native-vulkan-dual-031/`. Its old-library negative control failed
at probe code 57, where the previous library rejected a 17-pipeline call
(`old-library-rejection.log`). With the new library, that call exhausted the
current 16-page owner resource arena and returned out-of-device-memory with
all 17 output handles null. A subsequent 13-pipeline call filled the remaining
arena capacity successfully and destroyed those pipelines. The final
two-pipeline call then succeeded, verifying that the failed call released its
earlier allocations.

The two pipeline objects use distinct fragment programs. Their ordered N100
submissions produced exact image hashes `28616d2d` and `1629ed2d`; the mapped
image contained the second result. The output fence was signaled after both
completed. The visible vertex rotation (version 8), depth vertex program
(version 8), and two-draw original-program batch (version 7) also passed with
no fault (`physical.log`). Binary hashes are in `physical-hashes.log`. The
physical test lock was verified free afterward.

This extends the tested Vulkan API path to creating and using multiple
pipeline variants. Shader admission, image shape, and presentation are still
limited by the current native owner and userspace implementation.
