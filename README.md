# Native Intel Alder Lake-N graphics for RenkuOS

This project develops a native graphics driver and graphics stack for RenkuOS on Intel Alder Lake-N, with the Intel N100 (`8086:46d1`) as the primary hardware target. It includes display, GPU execution and experimental Vulkan work. This is an independent native development effort; an earlier Linux i915 port experiment is preserved as historical research.

## Current status

Physical N100 tests have demonstrated native GPU buffer copies and shader rendering. The experimental Vulkan implementation supports a limited set of workloads; recorded tests include image uploads, rendering and readback. This is research software, not a complete or production-ready GPU driver or Vulkan implementation.

- [Native GPU handoff](NATIVE_GPU_HANDOFF.md)
- [Native GPU implementation and copy evidence](experimental/native-gpu/README.md)
- [Physical shader rendering evidence and limitations](experimental/native-gpu/evidence/native-rcs-003/README.md)
- [Vulkan image layout and rendering evidence](experimental/native-gpu/evidence/native-vulkan-layout-103/README.md)
- [Vertex-color compiler experiment and remaining work](experimental/native-gpu/evidence/native-varying-105/README.md)

The evidence documents describe the exact tested revisions, configurations, failures and limits. Development continues; consult the latest commit and evidence rather than treating any single checkpoint as complete support.

## Background

Earlier display and memory work, hardware notes, patches and tests remain in this repository. The [Linux i915 port handoff](I915_PORT_HANDOFF.md) documents a superseded experiment and is not the current project direction. Source-derived code retains its original license and attribution; see [LICENSE](LICENSE) and the relevant source files.
