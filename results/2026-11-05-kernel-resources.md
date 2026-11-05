<!-- SPDX-License-Identifier: MIT -->
# Alder Lake kernel resource initialization

Source commit `4a143045`, patch 12, follows patch 7's disabled Alder Lake legacy
engine/overlay backend. Kernel initialization previously still requested a
64 KiB ring, one overlay page and the legacy status page. The latter fails
because it passes a null output pointer; patch 12 does not repair that unused
legacy interface. It skips all three requests on Alder Lake and preserves
independent cursor allocation. Other device groups retain their existing path.

This potentially avoids 68 KiB of unused aperture allocations, not a measured
hardware memory saving. It does not establish firmware/scanout ownership or
make physical installation safe. Remaining cursor/framebuffer allocations,
display/interrupt/fence setup and modern command submission need separate work.

The test extracts exact ring and auxiliary initialization statements from
`intel_extreme_init()`, retains source attribution and hashes, and compiles them
with mock device metadata and an allocator. Forty combinations cover Alder
Lake/legacy groups, physical/virtual overlay policy, cursor on/off and allocation
fault positions. **280 host ASan/UBSan checks pass.** The original source yields
exactly 56 failures: 20 unwanted-request, 16 nonempty-ring, 16 overlay-present
and four cursor-independence failures. These are fixture results, not actual
kernel initialization or hardware execution.

Bundle `renku-021` has 186 verified payload hashes. Full native compilation and
the same fixture/control are prepared, pending the Wi-Fi agent's build window.
No module is installed; the host graphics configuration is unchanged.

## Native validation and fixture correction

The first native build (`gpu-native-014`, bundle `renku-021`) compiled every
component, then exposed 16 cursor assertions in the test fixture. An optimized
host build without sanitizers reproduced them. The mock allocator wrote a
pointer field through an `addr_t*`, violating userspace aliasing rules; the
actual kernel recipe already uses `-fno-strict-aliasing`. The mock now copies
the address representation with `memcpy`. Production code was not changed to
accommodate the test. CI runs both sanitizer and optimized fixture variants;
both pass all 280 checks and reproduce the original 56 failures.

The next attempt (`gpu-native-015`) filled Fedora's filesystem while creating
the disk copy and never booted. After stopped-worker/no-VM verification under
the existing canonical flocks, its incomplete copy was removed. Normal
coordinator operation then reclaimed older GPU-only generated disks, preserving
all available logs/results and original source disks. The runner now measures
the clone footprint and requires an additional 2 GiB reserve before copying.

Fresh `gpu-native-016`, bundle `renku-022`, passed complete native manager,
Intel GART, Intel kernel driver and accelerant builds on hrev60072+518 with
matching SDK/GCC 13.3.0. All prior tests plus **280 kernel resource checks**
pass; the new original-code control produces exactly 56 expected failures.
All **186** payload hashes and ELF artifacts verify. The isolated VM shut down
cleanly and QEMU exited zero. No module was installed or loaded.

Manifest SHA-256: `c58698c87fd5cc716ad0ca6324137822fc20bf2ba78cf248b64dcc258a7949b0`.
Intel kernel driver: 243648 bytes,
SHA-256 `0b6a3377016659cc45d27bf07a283b559252aa456ecd6a120a30d547b056551c`.
Coordinator `astra-astra-gpu-f1d4d5b78cdd.service` succeeded in 140.263 seconds,
152.266 CPU seconds, 1.3 GiB peak cgroup memory, no swap recorded.
Evidence: `logs/renku-vm-014`, `logs/renku-vm-016` and corresponding Fedora
test directories. The failed disk-copy event is recorded separately because
the full filesystem prevented the coordinator's normal finalization log.
