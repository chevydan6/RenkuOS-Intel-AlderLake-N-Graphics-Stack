# Native N100 Vulkan FragCoord scale family

Date: 2026-11-15 (Asia/Tokyo). Physical target: `renkuos-n100`.
The known-good owner from the prior FragCoord checkpoint is backed up on the
N100 at `/boot/home/native-gradient-family-049/rollback/intel_memory_owner`.
`installed-hashes.log` records the new owner, unchanged bridge and compiler,
and userspace candidate binaries.

This increment moves the fragment shader admission rule from one exact program
to three Mesa-compiled programs: red = `gl_FragCoord.x / 32`, `/ 64`, or
`/ 128`. The 352-byte Intel programs share the same layout and differ only at
machine dwords 29 and 77. The owner checks those paired encoded immediates and
every other word, including zero padding through the whole 4 KB shader page.
It derives the denominator from the admitted program and verifies the rendered
pixels using Vulkan pixel-center coordinates and UNORM8 saturation. The
compiler-backed Vulkan pipeline accepts the same three compiled programs.

The test first ran the new userspace against the previous owner. The previous
owner rejected `/128` at submission (`old-owner-rejection.log`, exit 26,
`ioctl=-1`). After installing the new owner and rebooting, all 72 submissions
across 21 physical cases passed (`physical-matrix.log`): eight draws at each
FragCoord scale, 32×32 and 64×64 original/RGB triangles across four shapes,
and eight draws each with rotated and depth vertex shaders. Every trace has
`fault=0`, `observed=sequence`, and `matched=checked`. Each scale draw checked
3,476 pixels. `owner-vm-build.log` records the full owner CPU suite, and
`vulkan-vm-build.log` records the Mesa compiler pipeline probe.

| Scale | Full image FNV-1a | Pixel (20, 10) | Captured image SHA-256 |
|---|---|---|---|
| 1/32 | `655762f5` | `ffa30000` | `cf9cc867e549a0a7d3eb375362ab513f7d694e6cbd6059ebcecd65b17cd38346` |
| 1/64 | `7b2f62f5` | `ff520000` | Prior checkpoint |
| 1/128 | `a0a362f5` | `ff290000` | `08be1d9f181397526f85d10ca189754346513e94dd83c9b2e8b76303eb822d7d` |

`fragcoord32.bgra` and `fragcoord128.bgra` are raw 64×64 BGRA captures. All
4,096 pixels in each were independently compared with the prior verified
1/64 image's triangle footprint, replacing shader-output red with the
corresponding pixel-center formula and leaving background pixels unchanged.
`fragcoord32.fs.bin` and `fragcoord128.fs.bin` are the compiler outputs.

Reproduce on the installed N100:

```sh
ssh renkuos-n100 'LIBRARY_PATH=/boot/home/native-gradient-family-049:/boot/home/native-fragcoord-048:/boot/home/native-rcs-large-039/vulkan-deps:/boot/home/config/lib:/boot/system/lib RENKU_VK_TARGET_64=1 RENKU_VK_TRACE=1 /boot/home/native-gradient-family-049/triangle_probe 0 fragcoord128 8'
```

This remains a bounded shader family and synchronous triangle path, not
general Vulkan shader execution or a conformance claim. The selected Mesa
compiler source closure was 13,623,249 bytes in the preceding checkpoint;
this increment uses the same compiler binary, so it does not establish a new
whole-stack source or tokenizer total.
