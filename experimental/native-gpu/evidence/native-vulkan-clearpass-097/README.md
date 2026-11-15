# Native Vulkan CLEAR render pass, checkpoint 097

The userspace Vulkan driver now accepts `VK_ATTACHMENT_LOAD_OP_CLEAR` for its
single BGRA8 color attachment. `vkCmdBeginRenderPass` records one float color
value, converts it to BGRA8 UNORM, and marks the first draw of that pass.
Submission writes the clear color to the image BO through native resource
ioctls, then uses the existing preserving GPU draw request. This adds a
common application-visible render-pass operation without changing the N100
kernel owner or adding Linux graphics infrastructure.

The previous driver rejected the new probe at render-pass creation (code 18).
With the new driver, one targeted N100 run at 32×32 and one at 64×64 passed.
The trace records one clear operation (one or four image pages), owner request
v19, completed sequences, no fault, and matched owner pixel checks. Each
complete captured image equals the earlier GPU triangle image with every
background pixel replaced by the requested color `0xff336699`.
SHA-256 hashes are
`f2225634ab79767e9610e810f9e880d794ae4016e563850b61a1b5bc5b08621c`
and `5d9b3fa7ae8ab0954954955875eb424b85939a7ff6fd85a85fce3c84f5874fe7`.
`run-clear.py` reproduces the two hardware checks and full-image comparison;
`clear-hardware.log` and the raw captures retain the result. The Haiku VM
compiler pipeline, Vulkan API, fence, semaphore, and command-reset probes
also passed. `loaded-images.txt` records installed hashes; the kernel owner
remains checkpoint 096. The candidate Vulkan library has no libdrm dependency.

The selected runtime-source closure is approximately **14,816,375 bytes**:
the checkpoint-096 inventory plus the 2,187-byte change to its only modified
runtime-source file. `source-delta.json` records this calculation. This is
an adjusted byte inventory, not a fresh dependency scan, a complete RenkuOS
source total, or a model tokenizer count. The 3–5 MB target remains open.

Current limits: the clear pass must contain a draw; a clear-only pass is not
yet recorded as an independent command. A six-index single primitive cannot
be the first draw of a CLEAR pass because the current owner v24 request does
not preserve an existing image. The existing 32×32/64×64 target and shader
profile limits still apply. No Vulkan conformance is claimed.
