# Public fragment shader BO on physical N100, 2026-11-14

Render command version 6 accepts a file-local shader BO handle and one of the
four fixed triangle variants. The owner checks all 4 KiB of the BO: the first
208 bytes must equal the Mesa 26.3.0 fragment shader that passed the earlier
hardware regression, and the rest must be zero. It then patches exactly three
words of its proven command image: the instruction window to 32 pages and the
two PS SIMD kernel pointers to the owner-resolved BO address. The private FS
page is zeroed before submission, so a successful draw requires instruction
fetch from the public BO. The client supplies no GPU address or arbitrary
shader binary.

The [native build](native-build.log) passed, including CPU validation of the
full shader page and exact packet patch. A host ASan/UBSan run of that helper
also passed. Gen12 XML independently decoded the patched instruction range
and pointers. [Source hashes](source.sha256) identify the exact code and
shader. The installed owner SHA-256 is
`532bc04fcf5df2fe40326c6fec8b85377ea82592d5de7abd48e7241d0728aae9`;
bridge SHA-256 is
`8422ba1c18a0b3036af5409f08f1ae907c64598f635da1f851a0b12603bed242`.
See [installed hashes](installed-hashes.txt). The prior known-good v5 add-ons
are saved in `/boot/home/native-rcs-public-shader-014/`.

After reboot, a read-only probe, explicit bootstrap, and first v6 draw passed.
That draw fetched from public GPU address `0x410000`, checked 654/654 pixels,
returned hash `28616d2d`, and completed with no GPU fault.

| Physical regression | Result | Final sequence |
| --- | --- | ---: |
| [64 public shader v6 draws](v6-64/metadata.json) | Four expected image hashes, gap-free completion | 66 |
| [64 public batch v5 draws](v5-64/metadata.json) | Existing baseline preserved | 130 |
| [64 public vertex v4 draws](v4-64/metadata.json) | Existing baseline preserved | 194 |
| [64 separate page v3 draws](v3-64/metadata.json) | Existing baseline preserved | 258 |
| [64 user vertex v2 draws](v2-64/metadata.json) | Existing baseline preserved | 322 |
| [64 fixed v1 draws](v1-64/metadata.json) | Existing baseline preserved | 386 |
| [Four final v6 draws](v6-after-controls/metadata.json) | Public shader still passes | 390 |

Each runner preserved structured JSONL and raw client output. Four malformed
shader requests—changed code, nonzero trailing page data, a forged second-file
handle, and a stale handle—were rejected before submission. Resource
create/read/write/destroy and the final read-only probe passed; see
[final checks](final-negative-resource-probe.log). The physical test lock was
released; the Govee outlet remained on and was not cycled. No RenkuOS base
kernel source or Linux DRM/GEM interface was changed.

This proves GPU execution of a userspace-owned shader BO for one exact,
previously validated program. It does not yet accept arbitrary shader code,
compile SPIR-V on RenkuOS, submit general Vulkan command buffers, expose
asynchronous fences, bind dynamic GPU virtual memory, or present through a
Vulkan swapchain.
