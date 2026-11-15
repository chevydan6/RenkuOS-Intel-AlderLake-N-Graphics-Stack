# Prebound N100 resource arena expansion (069)

The native owner now prebinds 64 resource pages (256 KiB) at GPU VA
`0x410000..0x44ffff`, up from 16 pages (64 KiB). The Vulkan memory heap and
allocation limit follow the same range. The RCS batch admission and
`STATE_BASE_ADDRESS` instruction window now admit shaders and vertex data in
the expanded range. The pages are bound during owner startup; this does not
introduce live page-table edits or change the public owner ioctl ABI.

The new `highva` triangle variant reserves the old 64 KiB region before
allocating its vertex, image, and shader resources. It failed at vertex-memory
allocation with the previous owner ([red trace](old-owner-new-vulkan.log)).
With the expanded owner, [16 physical N100 draws](regression/highva-matrix.log)
passed for 32×32 and 64×64 images through direct and fallback paths. Their
structured traces show vertex and fragment shader GPU addresses at or above
`0x420000`, no fault, completed sequence IDs, and expected pixel hashes.

The [regression verifier](regression/verify.py) passed **206 prior draws across
68 cases** with this owner and Vulkan library. The new [arena runner]
(regression/run-arena.py) adds 8 ordinary fallback draws and the 16 high-VA
draws, checking addresses, completion, and pixel output. The dual-pipeline
probe's rollback case now requests 65 pipelines because 17 no longer exhausts
the expanded arena. The 64×64 direct and fallback high-VA cases passed.

The [owner VM build](owner-vm-build.log) passed its CPU smoke suite; the
[Vulkan VM build](vulkan-vm-build.log) passed compiler, API, fence, semaphore,
and reset probes. The [native bundle build](native-bundle-build.log) and
[physical hashes](physical-hashes.log) identify the exact tested binaries.
The installed owner is SHA-256
`1acd11fe6669df902b41ed8213c67fc71e3ad4bef68dacb0cf445077019b529a`.
The prior known-good owner is saved at
`/boot/home/native-highva-069/rollback/intel_memory_owner`.

The measured [selected canonical source closure](stack-manifest.json) is
**12,637,371 unique bytes**, 1,691 bytes above checkpoint 068. This excludes
SDK and generated inputs, probes, some external implementation source, and
the model-token count. The 3–5 MB experimental target remains open. The
resource arena is still fixed-size and prebound; generic Vulkan BO residency,
dynamic GPU VA mapping, and broader Vulkan shader support remain unfinished.
