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
