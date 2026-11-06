# Maintaining the N100 display path

The tested baseline is source tag `gpu-hdmi-metal-056` (commit `2580448c`).
It produced a visible, updating HDMI desktop on the physical N100. Cleanup
uses source branch `gpu/n100-cleanup` and project branch `gpu/cleanup-hdmi`.
Keep that baseline available for comparison and recovery.

## Responsibilities

| Layer | Files | Responsibility |
| --- | --- | --- |
| Haiku integration | `Ports.cpp`, `mode.cpp`, `dpms.cpp` | Dispatch, mode completion, display-power requests, framebuffer lifetime |
| Output lifetime | `AlderLakeHdmiContext.h` | One port's persistent state, raster start/stop, optional desktop completion |
| Hardware sequence | `AlderLakeHdmiSequence.h` | Ordered acquisition and reverse cleanup of dependent resources |
| Access policy | `AlderLakeTypeCMmio.h`, `AlderLakeCloneMmio.h` | Existing BAR bounds, supported offsets, constrained writes |
| Register definitions | `AlderLakeDisplayRegisters.h` | Shared physical offsets; not inherited register-block tags |
| Power and ownership | `AlderLakeDisplayPower.h`, `TypeCPowerLifecycle.h`, lease helpers | Shared references, ownership, readiness, cleanup retry |
| Clock and link | `DekelHdmi*`, `TypeCHdmiClock.h`, `TypeCFiaLaneCount.h` | PLL planning, PHY setup, port clock, lane routing |
| Raster | `AlderLakeHdmiPipe.h`, transcoder helpers | Disabled-pipe timing and format preparation, bounded enable/disable |
| Desktop plan | `AlderLakePlaneClone.h` | Pure clock, geometry, buffer and watermark validation |
| Desktop programming | `AlderLakeClonePlaneProgrammer.h` | External plane/scaler state and reversible register updates |

Pure planners must not perform I/O. MMIO adapters must not allocate or create
physical mappings. Port integration must not bypass either layer's validation.
The read-only power prerequisite helper is a diagnostic check; it does not own
power and cannot replace the power manager.

## Ownership and ordering

All new-path register transactions run under the shared accelerant lock.
Persistent port/pipe leases and power references outlive that transaction.
They are not substitutes for the lock. Clone teams must not destroy the primary
accelerant's shared locks.

The output sequence pins existing parent power, takes PHY ownership, requests
AUX power, routes FIA lanes, programs the PLL and port clock, requests DDI I/O
power, then prepares and enables the pipe/transcoder/PHY/buffer stages. Use the
sequence implementation as the authoritative order; do not reproduce it in a
second caller.

Stop the desktop plane and wait for its disabled state to latch before tearing
down its link. A failed stop must retain the framebuffer backing. A failed
cleanup retains its saved state and dependencies for retry: never delete that
object, free backing, or unmap registers merely because a call returned an error.
An active output intentionally holds resources; a positive `NeedsCleanup()`
alone does not mean the driver has failed.

## Hardware-specific invariants

- Preserve register fields the stage does not own. Use named masks and cite the
  relevant Intel or pinned Linux definition when adding an offset or field.
- Register offsets in the new adapters are physical BAR offsets. The inherited
  `read32` API also uses tagged offsets; do not interchange the two conventions.
- Display-version-13 pipe BPC and transcoder BPC are separate settings. The
  current path requires both to select RGB8.
- DDB register ranges are local to an MBUS slice set. Pipe A and B can have the
  same local range while using disjoint physical slices. Validate routing before
  reasoning about overlap; never add the global slice offset to the plane field.
- The desktop path supports the measured matching-mode configuration only. Keep
  its strict guards until broader modes have their own validated allocation,
  bandwidth and watermark support.
- Do not rewrite GGTT mappings or disable a safety check to work around a visual
  failure. Capture the failed stage and compare live hardware state first.

## Review and verification

Keep structural cleanup separate from changes to register values, ordering or
supported hardware. A cleanup change should preserve existing failure behavior
as well as the successful path. Use existing tests for ownership conflicts,
timeout, every-write rollback, readback failure and retained cleanup state.

Run the host lifecycle suite and context checks, then prepare and compile the
matching native bundle using `BUILD.md`. The native artifact finalizer must
pass. A VM verifies compilation and simulated-register behavior; it does not
verify the Intel output. Hardware validation uses the isolated USB, canonical
resource locks, backed-up artifacts and a fresh NanoKVM capture. Record actual
underrun counters and visible output, not just a successful mode-setting return.

The numbered patches preserve the research and tested milestones. Do not erase
that history during cleanup. A later upstream series should group coherent
changes and retain their regression tests and licensing attribution.

## Current limits

The path is experimental, opted in through explicit test markers and restricted
to the verified N100 configuration. Audio, arbitrary layouts, hotplug, repeated
power transitions and GPU rendering acceleration are not established by the
HDMI desktop test. Maintainability cleanup does not change those capabilities.
