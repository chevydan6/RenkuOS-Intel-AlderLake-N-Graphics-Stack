# Indexed Vulkan vertex offset, N100 checkpoint 099

`vkCmdDrawIndexed` now supports signed `vertexOffset` for the bounded N100
indexed-mesh path. The Vulkan driver shifts the bound vertex address by
`vertexOffset * 16` bytes before recording the existing owner v25 request.
Both directions are checked against the bound buffer, the 4 KiB page, and
the owner's existing 3–256 vertex limit. The owner and bridge binaries did
not change; no Linux graphics interface was added.

The new probe puts one decoy vertex before the nine-vertex mesh. A positive
offset uses binding offset 0 and `vertexOffset=1`; a negative offset uses
binding offset 32 and `vertexOffset=-1`. Both resolve to byte offset 16 and
render the same geometry. With the previous Vulkan library, the positive
case failed at `vkEndCommandBuffer` (probe code 25), before submission.

On the physical N100, both signs passed at 32×32. The positive case also
passed at 64×64. Full raw image captures matched the previously saved v25
mesh captures byte for byte:

| Case | SHA-256 of raw BGRA image |
|---|---|
| 32×32, negative offset | `87d977b759fdcb13f13541c87a123c830ca177b9d46c51b2510c56451d878f1d` |
| 64×64, positive offset | `3f45099273aee893f2430878c12fcd96a5cc345f1e680565b281b5217b1c7f62` |

The final candidate library also passed one ordinary nonindexed triangle.
With `RENKU_VK_TRACE=1`, the negative-offset submit reported
`vertex_offset=16`, `vertex_bias=-1`, owner v25 completion stage 12, and
`pixel_hash=cdad4a41`. The library was built on the Haiku VM with
`-Wall -Wextra -Werror` and exercised on the physical N100 from an isolated
bundle at `/boot/home/native-indexoffset-099`; the installed v25 owner was
left in place. Candidate library SHA-256:
`c72e8ba19ec5161ec441238c9d13554a6a341e511396e5574026273efa8b7407`.

Selected runtime source is **14,820,532 bytes** using the prior selected
closure plus the 631-byte increase in `NativeVulkan.cpp`. This is still a
partial source inventory, not a complete kernel plus userspace token count.
The 3–5 MB target remains unmet. This increment does not expand the fixed
shader, target format, image size, or presentation support. The full 262-draw
hardware suite was not repeated for this focused capability.
