# Selected GL-extension constants for the native Vulkan compiler (062)

The selected Mesa 26.3.0 Intel SPIR-V compiler still included the 860,150-byte
`GL/glext.h` after [checkpoint 055](../native-glheader-prune-055/README.md),
because `GL/gl.h` includes it internally. The [derivation script](../../render/compiler/mesa-26.3.0/native-glext.py)
uses `GL_GLEXT_LEGACY` and selects 430 constants referenced by the measured
compiler file closure. It preserves the Mesa and Khronos MIT notices in the
[derived headers](../../render/compiler/mesa-26.3.0/native-glheader-pruned/util/glheader.h).
This is a pinned, generated compiler input, not an OpenGL implementation.

The generator checks the SHA-256 of the 055 Mesa-derived `glheader.h`
(`bed967e399e9f0fde4d2821773c5280a122fecc8bfedeaf1fd53a02d75f1a057`)
and Mesa's Khronos `GL/glext.h`
(`86c4c01c82d2090c38551755851c0efda3acf18f2cce812e7a6ca93347110743`).
It scans the selected compiler files, follows references within constant
definitions, and emits a 20,921-byte constants header. Re-derivation on the
Haiku VM matched both checked-in headers byte for byte. Changing Mesa or the
compiler object selection requires regenerating and revalidating this header.

| Selected closure | 054 baseline | 055 | 062 |
|---|---:|---:|---:|
| Unique canonical runtime source | 14,174,197 B | 13,479,049 B | **12,628,736 B** |
| Selected Mesa source | 13,623,249 B | 12,924,143 B | 12,052,862 B |
| Compiler input files | 855 | 844 | 843 |
| Generated compiler inputs | 62,392,205 B | 62,392,205 B | 62,392,205 B |

The 062 runtime closure is **850,313 bytes smaller** than 055 and
**1,545,461 bytes smaller** than 054. `GL/glext.h` and `KHR/khrplatform.h`
leave the selected closure; the derived constants header enters it. The
[stack manifest](stack-after.json) contains file paths, byte counts, and
hashes. These are source bytes, not binary size or model tokens. Some external
implementation source remains outside the closure, and a model tokenizer is
not available here. The 3–5 MB target is still unmet.

The compiler linked the same 346 objects, has no libdrm runtime dependency,
and its binary SHA-256 is **identical** to the 055 compiler:
`1efaed46dc59b9a39ade10fa1aa6c604d146f68e7fca6373b22dfe560e632492`.
The [compiler report](compiler-report.json), [file list](compiler-files.txt),
and [build log](compiler-build.log) retain the Haiku VM result. The VM shader
probe accepted approved code and rejected changed instructions and invalid
input; its [build log](vulkan-vm-build.log) records the exact bundle hashes.

The isolated 062 bundle on the physical N100 passed **50 cases and 162 draws**:
124 direct-target matrix draws, 30 supplemental shader and command draws,
and 8 fallback draws. Every saved submission completed without a fault and
matched the expected pixel hash. Run `python3 verify.py` to check the
[matrix](physical-matrix.log), [supplemental](supplemental-matrix.log), and
[fallback](fallback.log) traces offline. The [physical hashes](physical-hashes.log)
identify the exact tested binaries. The installed owner and kernel were not
replaced. The intermediate full-macro 061 build also passed 124 draws with
the same compiler binary; its [trace](macro-full-matrix-061.log) is retained.

## Reproduce the VM input

Using the pinned Mesa tree, prior 055 compiler file list, and the native
compiler linker:

```sh
python3 native-glext.py \
  /boot/home/test-work/native-glheader-055/util/glheader.h \
  /boot/home/test-work/mesa-26.3.0/include/GL/glext.h \
  /boot/home/test-work/native-compiler-glheader-055c/files.txt \
  /boot/home/test-work/native-glheader-062
python3 link-native-compiler.py \
  /boot/home/test-work/build-compiler \
  /boot/home/test-work/native-fragcoord-runtime-048/intel_spirv_runtime.c \
  /boot/home/test-work/native-device-info-044/intel_device_info.c \
  /boot/home/test-work/closure-current-022/include \
  /boot/home/test-work/vulkan-direct-target-053/lib/libSPIRV-Tools-shared.so \
  /boot/home/test-work/native-compiler-glheader-062 \
  /boot/home/test-work/native-common-045/intel_common.c \
  /boot/home/test-work/native-glheader-062
```

On Haiku, set `LIBRARY_PATH` to the SPIRV-Tools library directory plus
`/boot/home/config/lib:/boot/system/lib` for these commands. The compiler
output directory must not already exist. The 062 source root is a build
candidate; the installed N100 driver is unchanged.
