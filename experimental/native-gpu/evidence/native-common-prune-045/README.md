# Native Intel common helper reduction (045)

Mesa 26.3.0's `src/intel/common/intel_common.c` contains `intel_common_update_device_info`, which queries a Linux DRM fd for engine information. The selected native compiler does not call it, but linking the original object selects i915 and xe engine transport objects and their DRM headers. `native-common.py` verifies the pinned Mesa source hash, retains Mesa's copyright and license notice, and derives a file without that fd query and its now-unused includes. The remaining compiler helpers are kept. `link-native-compiler.py` accepts this third native override after the existing runtime and PCI-ID-only device-info overrides.

## Build and test

The Haiku VM used Mesa `/boot/home/test-work/mesa-26.3.0`, build `/boot/home/test-work/build-compiler`, native runtime `/boot/home/test-work/closure-current-022/src/intel_spirv_runtime.c`, PCI-ID-only device info `/boot/home/test-work/native-device-info-044/intel_device_info.c`, and SPIR-V Tools `/boot/home/test-work/compiler-pipeline-restored-008/lib/libSPIRV-Tools-shared.so`. The derivation and link commands were:

```sh
LIBRARY_PATH=/boot/home/config/lib:/boot/system/lib python3 native-common.py /boot/home/test-work/mesa-26.3.0/src/intel/common/intel_common.c /boot/home/test-work/native-common-045/intel_common.c
LIBRARY_PATH=/boot/home/config/lib:/boot/system/lib:/boot/home/test-work/compiler-pipeline-restored-008/lib python3 link-native-compiler.py /boot/home/test-work/build-compiler /boot/home/test-work/closure-current-022/src/intel_spirv_runtime.c /boot/home/test-work/native-device-info-044/intel_device_info.c /boot/home/test-work/closure-current-022/include /boot/home/test-work/compiler-pipeline-restored-008/lib/libSPIRV-Tools-shared.so /boot/home/test-work/native-compiler-045 /boot/home/test-work/native-common-045/intel_common.c
```

The separate Vulkan candidate was built with `build-native-candidate.sh` against this compiler, then staged at `/boot/home/native-compiler-045/` on the physical N100. The installed kernel owner hash stayed `78b6c535e51c0bec2e0696d7a39896e0b87d72c049673c94389aba8fa1273e82`; there were no kernel changes. Exact source and binary hashes are in `source-hashes.log` and `physical-hashes.log`.

The compiler probe accepted bounded RGB and vertex programs and rejected changed instructions and invalid input. The N100 completed 35 traced submissions: 64×64 ordinary and RGB triangles for four shapes, two draws each; rotated-vertex and depth-vertex triangles, eight draws each; and three 32×32 checks. All 13 probe results passed, every reported pixel matched, every completion sequence was observed, and every submission reported `fault=0`. `physical.log` preserves the event records and image hashes.

## Source measurement

The compiler link selected 345 objects and 854 source/include files, down from 349 objects and 869 inputs in candidate 044. Selected Mesa source fell from 13,886,962 to 13,620,609 bytes. The native common derived source adds 9,949 bytes. The selected closure no longer includes `i915_drm.h`, `xe_drm.h`, or the i915/xe engine objects. The compiler still links SPIR-V Tools, C++ runtime libraries, and Haiku `libroot.so`; it has no `libdrm` runtime dependency.

Combined canonical runtime source for the selected compiler, Vulkan driver, and kernel owner is **14,168,555 unique bytes**, down **256,404 bytes** from candidate 044 and **321,826 bytes** from the original 043 inventory. See `stack-after.json`, `../native-compiler-prune-044/stack-after.json`, and `../native-stack-closure-043/stack-manifest.json`. The figure excludes generated build files, SDK headers, and probes. It still omits SPIR-V Tools implementation source, additional RenkuOS kernel sources, and development-model tokenizer tokens, so it is not a complete-stack size claim.

This adaptation is tied to the exact Mesa 26.3.0 source hashes. Mesa's copyright and license notices are retained in the generated C files; an upstream source change requires review before deriving them again.
