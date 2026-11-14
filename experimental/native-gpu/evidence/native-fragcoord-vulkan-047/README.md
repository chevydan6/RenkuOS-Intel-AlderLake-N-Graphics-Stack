# Native N100 Vulkan FragCoord gradient

Date: 2026-11-15 (Asia/Tokyo). Mesa compiler source: pinned 26.3.0.
Physical machine: `renkuos-n100`; Haiku VM used for builds and CPU probes.

This checkpoint adds one bounded Vulkan fragment program that reads
`gl_FragCoord.x` and writes `(x / 64, 0, 0, 1)` into a 64×64 BGRA image. The
SPIR-V is compiled at `vkCreateGraphicsPipelines` by the native Mesa Intel
compiler. The owner admits only the exact 352-byte machine program and checks
the completed image against a CPU reference. It is a real N100 GPU draw, not
general shader support or Vulkan conformance.

## Finding and correction

The first 336-byte compiled program executed, but the physical image matched
integer X rather than Vulkan pixel centers: 2,615 of 3,476 checked pixels
matched. `physical-first.log`, `physical-capture.log`, and
`failed-image.bgra` preserve the failure. A PS position-offset change did not
alter the image (`physical-success-first.log`, despite its historical name)
and was reverted. Inspection of Mesa's NIR path showed that
`nir_lower_wpos_center` must run after `brw_preprocess_nir` for this program.
The corrected compiler emits 352 bytes with SHA-256
`49ebe1107d6ff75895dc46ce203276357f9bfee3b96b80162198b9512e65834f`.
`fragcoord-center.fs.bin` is that exact output.

The final physical image is `fragcoord-image.bgra` (16,384 bytes, SHA-256
`dc2c47d5283f4605962ee09458dafd99b3d77b14122315179548cf1284541363`).
`physical-center-first.log` records 3,476/3,476 checked pixels matched, no
GPU fault, completed fence sequence, and full-image FNV-1a hash `7b2f62f5`.
The corrected probe checks this hash, an interior pixel, and the background.
`physical-center-eight.log` records eight more passing gradient draws. The
`physical-regression-matrix.log` records 48 passing draws across 18 cases:
32×32 and 64×64 original/RGB triangles for all four shapes, plus rotated and
depth vertex programs. Every trace reports `fault=0`, matched pixel counts,
and `observed=sequence`.

The Haiku VM passed `rcs_fragcoord_smoke`, `rcs_public_shader_smoke`, and the
compiler pipeline probe. `installed-hashes.log` identifies the five binaries
on the N100. `compiler-link-report.json` records the selected compiler link:
346 objects and 855 include/link files, including 13,623,249 Mesa source
bytes and 90,335 native override bytes. It links SPIRV-Tools and Haiku
libraries, with no `libdrm` or selected i915 DRM header. This is a selected
compiler closure, not a complete graphics-stack source or tokenizer count;
SPIRV-Tools implementation source remains outside this measurement.

## Reproduce the hardware draw

With the installed binaries identified in `installed-hashes.log`:

```sh
ssh renkuos-n100 'LIBRARY_PATH=/boot/home/native-fragcoord-048:/boot/home/native-rcs-large-039/vulkan-deps:/boot/home/config/lib:/boot/system/lib RENKU_VK_TARGET_64=1 RENKU_VK_TRACE=1 /boot/home/native-fragcoord-048/triangle_probe 0 fragcoord 8'
```

Set `RENKU_VK_CAPTURE_PIXELS=/boot/home/gradient.bgra` for an optional raw
16 KB image capture. The owner also returns the completed raw image on a
version-12 pixel-reference failure, while keeping the ioctl failure status.
