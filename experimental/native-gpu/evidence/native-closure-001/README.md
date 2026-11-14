# Native RCS compile-input measurement, 2026-11-14

The installed N100 owner and execution bridge were rebuilt with GCC `-MMD`.
The four saved depfiles identify direct C++ compile inputs for the owner,
bridge, native page bridge, and kernel C++ support object. The measurement
script queries the physical RenkuOS machine for each input's byte count and
SHA-256. It also includes the canonical owner baseline and Intel license as
two provenance files. `manifest.json` records every path and digest.

| Category | Files | Bytes |
| --- | ---: | ---: |
| Native bundle source and license | 45 | 211,520 |
| Canonical Haiku owner source and headers | 15 | 100,367 |
| Generated owner translation unit | 1 | 39,778 |
| Haiku SDK headers | 79 | 311,805 |
| **All recorded files** | **140** | **663,470** |

The generated owner translation unit includes the canonical owner source;
these figures count file inputs, not unique semantic source. SDK headers are
included because the build reads them. This is **not** the complete graphics
stack or a minimized implementation. It excludes the offline shader compiler,
Mesa ANV/NIR, build-only generators, tests, documentation, userspace Vulkan,
and native presentation. It is also not a model-token measurement. The actual
development-model tokenizer is unavailable here, so token fields are null.

To refresh from the same physical build after rerunning `build-execution.sh`:

```sh
scp 'renkuos-n100:/boot/home/native-rcs-resource-012/build/*.d' \
  experimental/native-gpu/evidence/native-closure-001/
python3 experimental/native-gpu/measure-native-closure.py \
  --deps experimental/native-gpu/evidence/native-closure-001 \
  --bundle /boot/home/native-rcs-resource-012 \
  --output experimental/native-gpu/evidence/native-closure-001/manifest.json
```

The build-only depfile flag did not change either installed binary. Owner
SHA-256 is `6d6abe0a6492a7e27f0a2c2d8046895319ab683b7678b521912697f9e0e08099`;
bridge SHA-256 is `d592c8bef427668cacb33338d208f42665fc6dda5ce1c49b862eeb989bf2b005`.
Both match the corresponding rebuilt binaries in the bundle. The physical
public-batch rendering regression remains recorded in `../rcs-public-batch-001/`.
