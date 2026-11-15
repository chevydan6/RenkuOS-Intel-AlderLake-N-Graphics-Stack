# N100 Vulkan multidraw, checkpoint 082

The native Vulkan command buffer now records two draws in one render pass.
The first draw uses the existing direct-target ABI (version 15). The second
uses version 19, which preserves the target and asks the owner to compare
outside-triangle pixels against a snapshot of the prior image. Both draws
execute through the RenkuOS owner and native RCS path. There is no Linux
graphics dependency or separate OpenGL path in this increment.

The previous installed owner rejected version 19 before writing a report;
`old-owner-rejection.log` records that expected pre-install result. The
revised owner and bridge passed the Haiku VM CPU build and regression suite.
After installation and reboot, the physical N100 produced:

| Target | Version 15 | Version 19 | Final hash | Image comparison |
| --- | ---: | ---: | --- | ---: |
| 32x32 | 777/777 pixels | 766/766 pixels | `225b1ba1` | 0 mismatches / 1024 |
| 64x64 | 3476/3476 pixels | 3552/3552 pixels | `bb1f873b` | 0 mismatches / 4096 |

The expected complete image is the first single-triangle capture with every
non-poison pixel from the second single-triangle capture overlaid. The second
triangle is complementary, so the comparison proves that the first draw's
pixels survive the second submission. The six raw pixel captures and trace
logs are retained here.

The installed owner and candidate Vulkan library then passed the prior 222
regression draws, 24 image-offset draws, and 16 memory-offset draws. The
reproduction scripts and per-case logs are included. `physical-hashes.log`
identifies the tested binaries. The shader compiler and SPIR-V Tools library
are the previously validated binaries; this increment does not change them.

The latest measured selected runtime source inventory was 15,222,168 unique
bytes at checkpoint 081. The changed runtime source files add an estimated
1,457 bytes against checkpoint 080's manifest, giving **15,223,625 bytes**
for the current working source. This is a delta estimate, not a rebuilt
dependency closure. It excludes SDK/generated files, probes, wider kernel
sources, build-only generators, and any unmeasured Haiku package closure.
The development-model tokenizer remains unavailable, so model tokens are
unmeasured. The 3–5 MB source objective remains unmet.
