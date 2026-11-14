# Installed v6 owner/bridge compile inputs, 2026-11-14

The four saved GCC depfiles from the physical N100 v6 build identify 141
direct compile-input files totaling **666,930 bytes**. The
[hash manifest](manifest.json) records each physical file, category, size,
and SHA-256. This is 3,460 bytes and one file above the earlier v5
[inventory](../native-closure-001/README.md); the new file is
`render/RcsPublicShader.h`. The native-bundle category now has 46 files and
214,980 bytes. Haiku owner source and headers (100,367 bytes), Haiku SDK
headers (311,805 bytes), and generated owner translation unit (39,778 bytes)
are unchanged from the earlier inventory.

The generated owner translation unit includes canonical owner code, so file
sizes double-count some semantic source. This inventory excludes the native
clients, tests, build-only generators, Mesa shader compiler, ANV, and any
future Vulkan userspace driver. It is neither a complete graphics-stack size
nor a development-model token measurement; the actual tokenizer is still
unavailable. Regenerate with `measure-native-closure.py` using this directory
as `--deps` and `/boot/home/native-rcs-public-shader-014` as `--bundle`.
