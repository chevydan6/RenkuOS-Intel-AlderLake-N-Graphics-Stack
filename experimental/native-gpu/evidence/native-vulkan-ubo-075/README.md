# Vulkan uniform-buffer triangle, checkpoint 075

## Hardware result

The physical Intel N100 rendered a 32×32 Vulkan triangle twice with one
`VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` at set 0, binding 0. The application
flushed two different RGBA values into the same mapped 16-byte buffer before
successive queue submissions. Both version-16 owner submissions completed,
each matched 777/777 checked pixels, and the image hashes changed from
`28616d2d` to `4979062d`. The final image readback matched the second color.
See `ubo-physical.log`.

The installed owner and execution bridge remain checkpoint 074. The compiler,
Vulkan library, and probe are isolated in `/boot/home/native-ubo-075` on the
N100. `physical-hashes.log` records all five binaries.

The same userspace candidate passed the prior physical Vulkan regression
matrix: 124 ordinary draws, 28 semaphore draws, 16 green-gradient draws,
30 supplemental draws, and 24 arena draws, including 16 high GPU-address
draws. See `regression/regression-summary.log` and its per-case logs.

## Compiler path

The prior adapter returned `RENKU_INTEL_COMPILE_UNSUPPORTED` for the valid
UBO shader; the new compiler-pipeline test exited 25 against it. The new
adapter lowers one fragment UBO at set 0/binding 0 to Intel binding-table
entry 1 before Mesa's `brw_compile`. It rejects other bindings and resource
reindexing. On the Haiku VM, it emitted the exact 336-byte fragment program
previously proven by the native owner, SHA-256
`03179b37bcd77cf5399f298622c91e9a46d55dc8f79e1a518fb85fc729636f92`.
The existing known shader tests also passed. Build logs are in
`compiler-vm-build.log` and `vulkan-vm-build.log`.

## Reproduce

```sh
ssh renkuos-n100 'LIBRARY_PATH=/boot/home/native-ubo-075:/boot/home/config/lib:/boot/system/lib RENKU_VK_TRACE=1 timeout 20 /boot/home/native-ubo-075/triangle_probe 0 ubo 2'
```

The regression scripts in `regression/` use the same isolated bundle.

## Source size and limits

`stack-manifest.json` measures **12,667,193 unique bytes** of selected
canonical runtime source for the pinned compiler, owner, and Vulkan build.
It excludes SDK, generated files, and test probes. This measurement corrects
the prior classifier so runtime shader headers are counted. The manifest
lists other unmeasured kernel and build inputs; the full 3–5 MB goal is not
met. The development model tokenizer is unavailable, so model tokens remain
unmeasured.

This increment supports one 16-byte fragment uniform descriptor with offset
zero and the proven 32×32 render path. Other descriptor layouts, buffer
offsets, and broader Vulkan behavior remain future work. No conformance
claim is made.
