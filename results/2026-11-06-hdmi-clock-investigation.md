# HDMI clock investigation

Physical test 004 read the HDMI sink's EDID but NanoKVM showed no image. The
log records a 1920×1080 mode and a port-G enable, but that does not establish
a working signal. During internal-panel setup the clock configuration changed
from `0x01e07801` to `0x00000001`; during port-G setup it changed from `0x1`
to `0x4`.

The Gen12 path in `DigitalDisplayInterface::SetDisplayMode()` used an AND with
the selected port's clock-select mask instead of its complement. This clears
unrelated fields, including clock-off control and fields for other ports, and
retains stale bits within the selection. Patch 15 corrects the two masks; it
does not change the current port-to-PLL assignment or implement a new PHY
sequence. A test compiles the actual register-writing block with mocked reads
and writes: 131,072 cases pass with the change, while the original fails all
131,072 tested inputs. No register access or physical driver test was run.

The physical VBT mapping used by RenkuOS identifies this connector as port G.
Linux treats the N100 as Alder Lake-P with display version 13; on that display
version its firmware port G mapping is TC2. Its register definitions restrict
the combo-PHY clock selection fields to combo PHYs and define separate Type-C
clock handling. RenkuOS currently assigns PLL 1 to port G and programs the
DDI B combo clock selection. The mapping and Type-C initialization therefore
need further investigation; the mask fix alone is not evidence of an HDMI
solution, and mapping a connector to Type-C hardware does not imply a physical
USB-C socket.

Linux's `icl_ddi_tc_enable_clock()` selects the port's MG or Thunderbolt clock
through `DDI_CLK_SEL` and clears the corresponding Type-C clock-off bit; it
does not substitute the combo-PHY B clock selection. A correct implementation
also depends on the applicable PHY, PLL, and ownership setup, so copying those
two register operations alone would be speculative. Next, map the firmware
connector and physical port to the appropriate Type-C PLL and PHY sequence,
including ownership, power, and timeouts, before testing a candidate.

Bundle `renku-025` compiled in the isolated RenkuOS hrev60072+518 VM; native
component checks and host finalization passed and the VM shut down cleanly.
The VM used emulated graphics and did not exercise the N100 hardware. The
patched accelerant SHA-256 is
`e64790decb30bcab3de9c5d9c92e682672feac16747d0cdcbdda5f2072e5626b`.
The clock-mask correction has not been installed on the laptop.

## Transcoder configuration gap

The physical log reports `trans DDI func ctl reg: 0x30000` for the selected
port-G pipe before setting its mode. Bit 31, the transcoder DDI-function
enable bit, is clear in this value. In the current RenkuOS accelerant source,
the relevant mode path reads `PIPE_DDI_FUNC_CTL` but has no corresponding
write to select or enable the external DDI function. `Pipe::Enable()` writes
the pipe and primary-plane controls, which are separate registers. Thus the
captured value and static source analysis give a further concrete reason to
investigate the missing external-transcoder configuration. The log does not
capture a final read-back of that register, so this is not proof of its final
state or the only cause of the missing HDMI picture.

Linux's `intel_ddi_enable_transcoder_func()` selects the port and enables the
DDI function; its display-version-13 clock selection uses the PHY associated
with the encoder. The preceding HDMI sequence also sets PHY signal levels and
powers lanes. Merely setting the enable bit without validated clock, port,
PHY, and sequencing is not a safe or complete fix. See Linux
[transcoder and HDMI setup](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_ddi.c)
and [transcoder register definitions](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_display_regs.h).

## Implementation progress

Patch 16 corrects the Gen12-and-newer transcoder port-selection field: it
uses four bits at bit 27, with zero representing no selected port. Both pipe
preference and connection detection use the corrected decode; older hardware
retains its previous decoding. The extracted function passed 280 host cases;
the earlier pipe-availability and clock-mask tests also passed. Bundle
`renku-026`, containing patches 1–16, compiled and passed native component
checks in a disposable RenkuOS VM, then passed host finalization. The VM shut
down cleanly. The new accelerant was not installed on the physical laptop.

`experiments/dekel-address` contains an unconnected C++ address calculation
for the 4 KiB banked Type-C PHY register aperture and HIP index. Its 104 host
checks pass with address and undefined-behavior sanitizers. It does not map or
access registers and is not built into the driver. Before real access, the
implementation still needs correct register-region mapping, serialization
across index selection and access, power and ownership handling, validated PLL
parameters and lock timeouts, and a complete transcoder and PHY sequence.

Patch 17 bounds the existing combo-PLL unlock, power-on and lock waits, and
returns `B_TIMED_OUT` to the port mode-setting function rather than spinning
forever. A host test exercises the actual PLL function with simulated success,
each timeout, and an invalid PLL. The 17-patch `renku-027` bundle compiled and
passed component tests in a disposable RenkuOS VM, and passed host
finalization. No physical driver installation or GPU execution was performed.
This is a bounded-wait safeguard, not a complete recovery mechanism: the
caller in `mode.cpp` currently logs port-mode errors and continues, and the
partially modified PLL state is not rolled back. Those issues require separate
design and validation before treating failure handling as safe.

## Power and ownership investigation

The existing RenkuOS accelerant has an AUX power-well helper, but it requests
only the fixed index zero and logs a timeout while still reporting success. It
does not provide a per-Type-C-port power-domain lifecycle or ownership
accounting. It cannot be reused as a complete TC2 power implementation.

The pinned Linux ADL-P path treats display-core power, the matching Type-C AUX
power domain, PHY readiness and display ownership as connected requirements.
It uses the TCSS DDI status readiness flag and an ownership bit in the port's
DDI buffer control; the AUX power path also checks PHY microcontroller health.
The precise AUX channel and register mapping must be confirmed from the
machine's VBT and compatible Renku register blocks. Failure and disconnect
must release only resources acquired by this driver, without disrupting the
internal display or another user of a shared power domain. See Linux
[Type-C handling](https://github.com/torvalds/linux/blob/f66469d9905b39d2c260a15d7d6db450f4db67c1/drivers/gpu/drm/i915/display/intel_tc.c)
and [power-well handling](https://github.com/torvalds/linux/blob/f66469d9905b39d2c260a15d7d6db450f4db67c1/drivers/gpu/drm/i915/display/intel_display_power_well.c).
No Type-C power or ownership writes were added or executed in this step.

Patch 18 adds a callback-driven Type-C power lifecycle to the accelerant
source. It tracks the power reference and whether it personally acquired PHY
ownership, preserves pre-existing ownership, and retains state for a cleanup
retry when a release fails. Its backend contract requires callers to serialize
operations across shared resources and requires failed mutations to leave the
resource unchanged. Eleven simulated acquisition, rollback, invalid-backend,
and retry cases pass both on the host with sanitizers and in a RenkuOS VM.
The 18-patch `renku-028` bundle built and passed native component checks and
host finalization; the VM shut down cleanly.

The lifecycle is not yet instantiated in the driver and has no physical
register backend. It does not implement power-well refcounting, PHY readiness
polling, TC-cold blocking, microcontroller-health checks, or the HDMI enable
sequence; it cannot power the port or fix HDMI by itself. No physical hardware
was changed or tested for this patch.

References: Linux [display-device classification](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_display_device.c),
[VBT-to-port mapping](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_bios.c),
[display register definitions](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_display_regs.h),
and [DDI clock handling](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_ddi.c).
