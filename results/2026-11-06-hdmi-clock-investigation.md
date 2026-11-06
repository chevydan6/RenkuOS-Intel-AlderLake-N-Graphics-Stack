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

References: Linux [display-device classification](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_display_device.c),
[VBT-to-port mapping](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_bios.c),
[display register definitions](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_display_regs.h),
and [DDI clock handling](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_ddi.c).
