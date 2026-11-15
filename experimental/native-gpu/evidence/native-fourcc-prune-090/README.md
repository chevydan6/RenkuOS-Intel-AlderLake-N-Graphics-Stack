# Native Intel format header, checkpoint 090

Mesa 26.3.0's selected compiler path included `drm_fourcc.h`, `drm.h`, and
`drm_mode.h` through `src/intel/isl/isl.h`. The linked compiler used four
format-modifier constants from those headers. A native header now supplies
those exact values without including Linux DRM UAPI. C++ static assertions
against the original Mesa header confirmed all four values.

`link-native-compiler.py` recompiles the union of selected objects that
included Mesa's broad GL header or DRM format header with the native header
root. The Haiku VM build linked 345 objects and 839 input files. Its runtime
dependencies are SPIRV-Tools, the C++ runtime, GCC runtime, and `libroot`;
there is no `libdrm` dependency. The three DRM UAPI headers are absent from
`compiler-files.txt`. The resulting shared library is 14,564,992 bytes,
unchanged in file size from checkpoint 080; binary size is unrelated to the
source reduction. `build.log.gz`, `report.json`, the object list, and the input
file list preserve the build evidence.

The three removed headers total **171,414 source bytes**. The replacement is
1,713 bytes. The two GL override headers moved to the combined VM staging
root with byte-identical SHA-256 hashes, so the selected-source delta is
**169,701 fewer bytes**. `stack-manifest.json` remeasures the current compiler
file list with checkpoint 089's owner and Vulkan manifests, yielding
12,352,701 unique runtime bytes. `spirv-tools-closure.json` adds the pinned
host reproduction of SPIRV-Tools and SPIRV-Headers, giving **15,072,193
bytes**, down from 15,241,894. This is still above the 3–5 MB target. It is a runtime
source inventory, not the complete build or operating-system dependency
closure, and no development-model tokenizer is available for a token count.

On the physical N100, `check-images` opened the existing Vulkan library and
reported `/boot/home/native-fourcc-090/librenku_intel_compiler.so` as the
loaded compiler. That candidate's SHA-256 is
`648bcdde044d2c71a851867b037a5d1fad7ccfa4b96a1b3673cc233b4be1d32d`.
The installed owner, bridge, and Vulkan library stayed at checkpoint 088.

With the candidate compiler loaded, the previous physical regression matrix
passed **262 draws**: 124 base, 28 semaphore, 16 green-gradient, 24 arena,
30 supplemental, 24 image-offset, and 16 memory-offset draws. The six
four-triangle runs also passed at 32×32 and 64×64, using both one and four
`vkCmdDraw` calls plus the unbatched fallback. The pair path used owner
versions 20 and 21, and complete pixel captures matched checkpoint 088's
baseline byte for byte. Final pixel hashes were `5bf758b9` and `61162dae`.
`regression/`, `hardware.log`, and the six raw captures preserve the results.

The native header retains the Intel copyright and license notice from Mesa's
DRM format header. The replacement defines only values used by this selected
compiler path; future Mesa updates must be checked for additional modifier
references before rebuilding against it. No kernel code changed in this
checkpoint.
