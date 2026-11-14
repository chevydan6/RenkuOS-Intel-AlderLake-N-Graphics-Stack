# Native compiler device-info reduction (044)

Mesa 26.3.0's compiler runtime uses PCI ID `0x46d1` to initialize Alder Lake-N device information. It does not call the Linux DRM file-descriptor device query. `native-device-info.py` checks the pinned upstream source hash and derives a source file that retains the PCI-ID path and Mesa's copyright notice while omitting the unused DRM fd query and its transport includes. `link-native-compiler.py` replaces only this object and the existing native runtime object in the selected compiler link. No kernel code changed.

## Reproduce

The original Mesa source is `/boot/home/test-work/mesa-26.3.0/src/intel/dev/intel_device_info.c` on the Haiku VM. Generate the native adaptation with:

```sh
python3 native-device-info.py /boot/home/test-work/mesa-26.3.0/src/intel/dev/intel_device_info.c /boot/home/test-work/native-device-info-044/intel_device_info.c
```

`link-native-compiler.py` uses `/boot/home/test-work/build-compiler/compile_commands.json` and `build.ninja`, the current native runtime at `/boot/home/test-work/closure-current-022/src/intel_spirv_runtime.c`, and the existing SPIR-V Tools shared library. `build-native-candidate.sh` builds a separate Vulkan loader and probes against the resulting compiler. The candidate was staged on the physical N100 at `/boot/home/native-compiler-044/`; the prior compiler and kernel owner stayed in place. See `source-hashes.log` and `physical-hashes.log` for exact hashes.

## Results

The N100 compiler probe accepted the bounded RGB and vertex programs and rejected changed instructions and invalid input. The physical render run passed 64×64 ordinary and RGB triangles across four shapes, two draws each; rotated-vertex and depth-vertex variants, eight draws each; and three 32×32 render checks. The captured submission records report matching pixels and `fault=0`. Full output is in `physical.log`.

The selected compiler link fell from 352 to 349 objects and from 878 to 869 source/include inputs. Mesa canonical inputs fell from 14,025,279 to 13,886,962 bytes. The 72,895-byte derived device-info file is included as native adaptation source. Combined runtime canonical source across compiler, Vulkan driver, and kernel owner fell from 14,490,381 to **14,424,959 unique bytes**, a **65,422-byte reduction**. The comparable measurement is `stack-after.json` against `../native-stack-closure-043/stack-manifest.json`. Generated build files, SDK headers, and test probes are excluded from this runtime source figure. SPIR-V Tools implementation source, additional kernel sources, and model tokens remain unmeasured.

The linked compiler has no `libdrm` runtime dependency. The selected source closure still includes `i915_drm.h` through Mesa's Intel engine helper, so this experiment removes the unused device-info transport path, not all Linux-derived source dependencies. The derived code remains subject to Mesa's original file notices and license.
