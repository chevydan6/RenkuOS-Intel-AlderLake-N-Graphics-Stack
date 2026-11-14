# Combined public vertex and shader draw on physical N100, 2026-11-14

Render command version 7 accepts two distinct file-local BO handles: `variant`
names the vertex BO and `reserved` names the fragment shader BO. The owner
validates all three position vectors and the entire 4 KiB shader page, then
patches only the proven vertex-buffer address, instruction window, and two PS
kernel pointers in its fixed Mesa 26.3.0 command image. It zeros the private
vertex and fragment shader pages before execution. The client supplies no GPU
address or arbitrary command or shader bytes.

The [client](../../gpu_render_combined.cpp) first rejects reuse of one BO for
both roles, then submits version 7 and verifies the full 32×32 image,
completion sequence, nonce, fault state, and owner pixel report. Against the
installed version 6 owner, the [red test](pre-v7-rejection.log) exited 1 with
stage 0 and sequence 0. The new bounded batch helper passed its CPU smoke and
host ASan/UBSan run. The [native build](native-build.log) and
[rebuild](rebuild.log) passed; the latter produced byte-identical binaries.

After installing only the owner and execution-bridge add-ons, a controlled
reboot loaded the new code. A read-only probe and explicit bootstrap passed.
The first version 7 draw returned 777/777 checked pixels, hash `28616d2d`,
matching completion sequence 2/2, and no GPU fault. Reproducible regressions:

| Workload | Draws | Final sequence |
| --- | ---: | ---: |
| [Combined version 7](v7-64/metadata.json) | 64 | 66 |
| [Public shader version 6](v6-64/metadata.json) | 64 | 130 |
| [Public batch version 5](v5-64/metadata.json) | 64 | 194 |
| [Public vertex version 4](v4-64/metadata.json) | 64 | 258 |
| [Separate vertex page version 3](v3-64/metadata.json) | 64 | 322 |
| [User vertices version 2](v2-64/metadata.json) | 64 | 386 |
| [Fixed version 1](v1-64/metadata.json) | 64 | 450 |
| [Final combined version 7](v7-after-controls/metadata.json) | 4 | 454 |

The [final checks](final-vulkan-negative-probe.log) passed 16 native Vulkan
ABI probes, four malformed public-shader cases, four malformed public-batch
cases, and a read-only RCS probe. The version 6 add-ons remain saved in
`/boot/home/native-rcs-combined-015/rollback/`. The Govee outlet stayed on;
no power cycle was needed. The physical test lock was released. No base
RenkuOS kernel source or Linux DRM/GEM interface changed.

The installed owner SHA-256 is
`f43c196dce2cb0404b75ce7601b48644b6d76a774503e8e250be1db92140108b`;
the bridge SHA-256 is
`3c627564f538d62e87342e3da6968b4843170ee14044e1bc2c6d1a916c22680f`.
See [installed hashes](installed-binaries.sha256) and [source hashes](source.sha256).
The four GCC depfiles and [manifest](manifest.json) inventory **141 direct
owner/bridge compile inputs totaling 668,490 bytes**, including 216,540 bytes
of native-bundle files. This is 1,560 bytes above the version 6 inventory,
with the same input-file count. The generated owner translation unit includes
canonical owner code, so this byte count double-counts some semantics. It
excludes clients, tests, Mesa compiler, Vulkan userspace, build-only scripts,
and model tokens.

This establishes a hardware-validated owner contract for a Vulkan triangle's
two input resources. The Vulkan library has not yet created a graphics
pipeline, recorded a draw, submitted version 7, or returned pixels through a
Vulkan image. The fixed validator still rejects general shaders and commands.
