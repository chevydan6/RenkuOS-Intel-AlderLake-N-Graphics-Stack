<!-- SPDX-License-Identifier: MIT -->
# Vulkan format discovery for the bounded N100 path

The native library now exposes `vkGetPhysicalDeviceFormatProperties` and
`vkGetPhysicalDeviceImageFormatProperties` through instance dispatch. It
reports linear BGRA color-attachment and transfer-source features, float4
vertex-buffer features, and a 32×32, single-layer, single-sample image limit.
Unsupported formats and usages return no features or
`VK_ERROR_FORMAT_NOT_SUPPORTED`.

The VM compiler/API probes passed after a deliberate red run: before the
implementation, the new API probe exited 16 because both dispatch entries
were absent. The final [VM build log](vm-build.log) includes the passing
compiler, API, fence, and command-reset checks. The physical N100
[standard image trace](physical.log) records draw sequence 55 and hash
`28616d2d`; [color-only image trace](physical-coloronly.log) records sequence
56 and the same hash, with 777 matching pixels and no reported GPU fault.
Both tests used the isolated `/boot/home/native-vulkan-format-035` bundle.
The installed owner, bridge, compiler, and kernel were unchanged.

Vulkan's `maxExtent` can report the current upper bound but cannot express
this experiment's **exact** 32×32 creation requirement. Image dimensions,
tiling, presentation, and shader/format support are still much narrower than
general Vulkan 1.0. This is capability discovery for the current path, not a
conformance claim.
