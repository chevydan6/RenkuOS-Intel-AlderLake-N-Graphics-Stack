# Validation and recovery

Current safe tests inspect source tables and query a fully linked accelerant
with synthetic shared state. The test never calls B_INIT_ACCELERANT, drawing,
MMIO, modesetting or submission. Native GART linking does not load that module.
These tests cover discovery data and capability exposure, not GPU execution.
The GGTT helper has 14 C++ checks for valid offsets, addresses above 4 GiB,
BAR bounds, zero/oversized tables, null output and end-address overflow.
Run on Linux with address/undefined-behavior sanitizers and in the native guest.
The Alder Lake PTE helper adds 28 checks for address width, alignment, bounds,
unchanged outputs on rejection and guards around actual 64-bit stores in RAM.
The GMS decoder checks all 65,536 register values against an explicit datasheet
table, plus null-output handling. These do not execute GPU commands or establish
ownership of firmware-reserved physical memory.

Before any install, use an isolated test image and preserve a powered-off,
flattened known-good base. Never copy a running qcow2 with an ordinary file copy
or use a mutable shared disk as an overlay backing file. A snapshot requires a
coordinated guest stop or supported QMP/block backup operation. The Wi-Fi agent
owns shared VM scheduling; GPU tests must not interrupt its runs.

For physical testing, obtain a window and explicit authorization for GPU/boot
changes. Verify out-of-band control and cold-boot recovery, retaining Fedora's
working boot entry and a separate Haiku/RenkuOS test medium. Capture PCI IDs,
firmware version, connector/EDID, source/binary hashes, boot log and syslog. Test
stock behavior first. Do not install the GART ID patch alone: the GGTT audit
found an incorrect offset and incompatible page-table writes in the inherited
Gen8+ path. Complete page-table width, allocation bounds, stolen-memory ownership
and initialization review before proposing any physical patched-driver trial.
Record graphics device node, aperture sizes, native eDP 1920x1080, HDMI modes,
refresh rate, cursor, repeated mode transitions and recovery to the baseline.
Do not load an experimental module over an active instance.

GPU passthrough is optional and currently unapproved. The active iGPU, RMRR,
reset limitations, OpRegion/VBT, stolen memory and emulated host bridge all need
review. Do not spoof bridge IDs to bypass GART discovery. Headless command tests
still need the physical GPU and correct memory/context initialization.

Later execution tests must verify output bytes against CPU reference data,
guard regions, bounded completion, non-overlap/overlap handling, ownership errors,
allocation failures, process exit, hangs and reset. Retain serial/log output after
failures. Stop on corruption; a timeout is a failed test, never a success.
