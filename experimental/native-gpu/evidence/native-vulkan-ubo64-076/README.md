# Vulkan UBO at 64×64, checkpoint 076

The physical N100 rendered a 64×64 triangle twice with a fragment uniform
buffer at Vulkan set 0, binding 0. The owner and Vulkan paths both used the
existing version-16 request and shader program. Each draw matched 3,476 of
3,476 checked pixels; the two image hashes were `8aaee2f5` and `8df12ff5`.
The second color's raw 16,384-byte image is `ubo64-capture.bin` (SHA-256
`b1de60eb854ba90e72639b092820166918e2c363698e7facfaa136a478246eec`),
with FNV-1a pixel hash `8df12ff5`. See `native64.log`, `vulkan64.log`, and
`vulkan64-capture.log`. The 32×32 Vulkan UBO path also passed; see
`vulkan32.log`.

The earlier Vulkan library rejected the 64×64 UBO pipeline at application
code 21, while the owner-side 64×64 draw already succeeded. The Vulkan change
admits that tested size and includes version 16 in the large pixel-capture
path. The compiler and installed owner/bridge are unchanged. The candidate is
isolated at `/boot/home/native-ubo-076` on the N100; `physical-hashes.log`
identifies its binaries.

The same candidate passed 222 existing physical regression draws: 124
ordinary, 28 semaphore, 16 green-gradient, 30 supplemental, and 24 arena
draws, including 16 high GPU-address draws. See the `regression/` logs.

Reproduce the Vulkan result:

```sh
ssh renkuos-n100 'LIBRARY_PATH=/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib RENKU_VK_TRACE=1 RENKU_VK_TARGET_64=1 timeout 20 /boot/home/native-ubo-076/triangle_probe 0 ubo 2'
```

`stack-manifest.json` measures **12,667,163 unique bytes** of selected
canonical runtime source. It excludes SDK, generated files, and probes. The
manifest lists broader kernel/build inputs not yet included, and the
development-model tokenizer is unavailable, so this is not a complete source
or model-token total. The 3–5 MB source target remains unmet. No Vulkan
conformance claim is made.
