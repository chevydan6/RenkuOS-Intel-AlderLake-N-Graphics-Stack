# Two compiled Vulkan fragment shaders on the physical N100

Date: 2026-11-15 (Asia/Tokyo). Branch: `codex/i915-native-adaptation`.
Device: Intel `8086:46d1`, RenkuOS `hrev60072+518`.

After [BFS recovery](../native-ramfs-unmount-008/README.md), `/boot` mounted
writable. The known-good version 7 owner and original Vulkan triangle passed
before this increment. All new work was staged in separate directories:
`/boot/home/native-rcs-two-shaders-013` and
`/boot/home/native-vulkan-two-shaders-014`. The seven modified owner source
files and ten Vulkan/compiler interface files in those directories have
[matching canonical source hashes](owner-sources.sha256) and
[Vulkan source hashes](vulkan-sources.sha256).

The physical [owner build](owner-build.log) passed its CPU and fake-engine
suite. The installed owner add-on was copied to
`/boot/home/native-rcs-two-shaders-013/rollback/intel_memory_owner.pre-two-shader`
before replacement. Only the owner add-on was replaced, with SHA-256
`a00cbf15ae53c07b0f0f2214a60fd1cbe28c15b448e4e7fa992a5e7e208b7b42`.
The existing execution bridge retained SHA-256
`3c627564f538d62e87342e3da6968b4843170ee14044e1bc2c6d1a916c22680f`.
The base kernel was not changed. A controlled reboot loaded the new owner.
Its read-only RCS probe and explicit bootstrap passed. The original owner
draw and all four original Vulkan triangle hashes passed after reboot; the
[final controls](known-good-control.log) passed again after the new tests.

The pinned Mesa 26.3 compiler library and SPIRV-Tools library were copied
from the VM only after checking their hashes. The VM's earlier scratch
Vulkan sources and one probe binary were inconsistent after a disposable VM
crash, so the physical [compiler build](compiler-build.log) used fresh source
from this repository. Its compiler, API, fence, and command reset probes
passed. The physical compiler-backed `libvulkan.so` SHA-256 is
`a2bb13cbc9ce446eb552ee148603244d82322c211c6b058cc3f70bd9bf8d9ed2`.
The full [binary and boot state](physical-state.log) is recorded separately.

The first half-green triangle passed for shape 0. The original half-green
probe then failed for shape 1 because its sampled pixel `(20,20)` was outside
that triangle; a diagnostic run found the clear value `0x19537bad` there,
242 green pixels elsewhere, and full-image hash `0bf039c3`. The probe now
samples interior pixels and requires exact full-image hashes. The
[128-draw physical regression](physical-128-draws.log) passed 16 repeats of
each original and half-green shape:

| Shape | Original hash | Half-green hash |
| ---: | --- | --- |
| 0 | `28616d2d` | `1629ed2d` |
| 1 | `d40eb9c3` | `0bf039c3` |
| 2 | `e24aed05` | `48e0ed05` |
| 3 | `984ae945` | `05d66945` |

An opt-in [half-green submission trace](half-trace.jsonl) for shape 2
reported completion sequence `222/222`, `766/766` matching pixels, hash
`48e0ed05`, no GPU fault, engine stage 4, and no CSB error. The normal path
does not emit this trace. The physical-test lease was released after tests.

This is hardware proof for two exact fragment programs in a fixed Vulkan
triangle path. Arbitrary SPIR-V pipelines, broader Vulkan behavior,
presentation, asynchronous GPU fences, and conformance remain unproven.
