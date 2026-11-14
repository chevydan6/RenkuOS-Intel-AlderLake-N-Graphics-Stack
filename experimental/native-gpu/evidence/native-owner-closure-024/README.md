# Current N100 owner direct-source closure (physical hardware)

The physical N100's staged RGB owner checkpoint has binary
SHA-256 `fdd85665ff0ee1989285fcf9e7796598529b94bf3d6ad8fb266c8a7232d2a075`.
This measurement used its staged build at `/boot/home/native-rcs-rgb-021`
read-only; no kernel component was rebuilt or installed.

`deps/` contains the four owner/bridge GCC dependency files. `manifest.json`
hashes and counts 141 distinct direct source/header inputs plus two provenance
files selected by the measurement script: 669,836 bytes. The native bundle
contributes 217,886 bytes, the generated owner translation unit 39,778 bytes,
the original Haiku owner source/headers 100,367 bytes, and Haiku SDK headers
311,805 bytes. The generated owner translation unit includes canonical owner
code, so summing all categories double-counts some logic.

This is a build-input inventory for the staged hardware-tested owner, not a count
of all RenkuOS kernel sources or a minimum implementation. It excludes Mesa,
SPIRV-Tools, Vulkan userspace, build-only generators, and the model-token
measurement. The physical rendering and repeat tests for this owner are in
[`native-color-compiler-spike-021`](../native-color-compiler-spike-021/README.md).
