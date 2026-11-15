# Native compiler GL-header closure experiment (055)

Mesa 26.3.0's `src/util/glheader.h` brings GLES declarations into the selected
Intel SPIR-V compiler even though the native Vulkan pipeline does not use a
GLES API. The pinned [derivation script](../../render/compiler/mesa-26.3.0/native-glheader.py)
replaces the GLES include block with the two enum values needed by selected
compiler source. The [derived header](../../render/compiler/mesa-26.3.0/native-glheader/util/glheader.h)
retains the Mesa copyright and license notice. It does not add GLES support.
The source header SHA-256 is
`def3c9be62583645c6ecb0f9579414a75aed664fca4fa2d20049f3a54ba7f184`;
the derived header SHA-256 is
`bed967e399e9f0fde4d2821773c5280a122fecc8bfedeaf1fd53a02d75f1a057`.

The native compiler linker rebuilds the 76 selected Mesa objects whose Ninja
dependencies include the original header, plus the native SPIR-V runtime,
using the derived header. It verifies that no selected compiler file still
depends on the original header and that the binary has no libdrm runtime
dependency. The link still selects 346 objects. [Compiler report](compiler-report.json),
[object list](compiler-objects.txt), [file list](compiler-files.txt), and
[build log](compiler-build.log) preserve the VM result.

| Selected closure | Before (054) | Candidate (055) | Difference |
|---|---:|---:|---:|
| Unique canonical runtime source | 14,174,197 B | 13,479,049 B | −695,148 B |
| Selected Mesa source | 13,623,249 B | 12,924,143 B | −699,106 B |
| Compiler source/input files | 855 | 844 | −11 net |
| Compiler generated inputs | 62,392,205 B | 62,392,205 B | 0 |

The 12 removed files are 11 GLES headers and Mesa's broad `glheader.h`; one
derived header is added. GL headers remain. [Stack manifest](stack-after.json)
records the candidate source paths and hashes; its owner and Vulkan source
manifests are the unchanged 054 inputs. Source bytes are distinct from the
14,594,272-byte compiler binary and from model tokens. The model tokenizer
is unavailable here, so no token count is claimed. This is still above the
3–5 MB experimental source objective and excludes some external implementation
source described in [054](../native-stack-current-054/README.md).

On the physical Intel N100, the isolated candidate compiler and Vulkan bundle
passed the compiler pipeline probe, 39 direct-target matrix cases (124 draws),
9 supplemental cases (30 draws), and 2 fallback cases (8 draws). Each GPU
submission completed without an ioctl error or fault and matched the expected
pixel hash. [Saved traces](physical-matrix.log), [supplemental traces](supplemental-matrix.log),
[fallback traces](fallback.log), and [offline verifier](verify.py) preserve
the evidence. Run `python3 verify.py` to recheck the 162 draws. The
[physical bundle hashes](physical-hashes.log) identify the exact tested files.
No kernel or installed owner file was replaced during this experiment.

The VM input paths were `/boot/home/test-work/mesa-26.3.0`,
`/boot/home/test-work/build-compiler`, and
`/boot/home/test-work/native-glheader-055`; output was
`/boot/home/test-work/native-compiler-glheader-055c`. The last positional
argument to `link-native-compiler.py` selects the derived header root. The
candidate Vulkan bundle was built from the existing Vulkan sources with that
compiler, then copied unchanged to `/boot/home/native-glheader-055` on the
N100. [VM probe result](vulkan-vm-build.log) records shader acceptance and
rejection before physical testing.
