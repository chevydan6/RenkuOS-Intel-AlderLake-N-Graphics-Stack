# HDMI integration development

Native builds 041–045 passed compilation, available simulated-register tests
and artifact finalization. Patches 43–48 add staged pipe timing/enable handling,
read-only power prerequisites, shared display-pipe leases, public DPMS locking,
conservative display power pinning, sequence lifetime composition, and live
modesetting diagnostics. These do not connect the experimental sequence to
physical output.

Build 044 was installed on both kernel-driver override paths and the accelerant
path of the separate RenkuTest USB. Previous files were backed up under
`/boot/home/gpu-044-recovery`. The laptop rebooted successfully from that USB.
The live diagnostic reported TC2, pipe index 1, reference 38400 kHz, preset 0,
DC_STATE_EN=0, MAIN=0x0000fc0f and DDI_IO=0x00000003. Required TC2 DDI_IO power
was absent; the main display/pipe domains were already on. The experimental
sequence remained disabled. No HDMI picture or acceleration was demonstrated.

NanoKVM's read-only `/proc/cvitek/vi_dbg` showed VIDevFPS=0 and VIFPS=0.
[Sipeed's diagnostic documentation](https://github.com/sipeed/sipeed_wiki/blob/main/docs/hardware/en/kvm/NanoKVM/faq.md)
identifies zero VIDevFPS as no incoming HDMI input reaching the capture path.

Patches 49–50 add an explicit RGB8 DVI-compatible TMDS plan limited to 165 MHz
and verified-N100 fixes for the plane offset register and pixel-format mask.
The plane functions passed 40 host fixtures; the original implementation
reproduced 26 failures. Build 045 passed native compilation. Patch 51 adds
bounded, power-gated read-only plane/DDB/watermark diagnostics; build 046 is
being validated before the next physical boot. It has not been tested on the
laptop at this checkpoint.

Build 046 subsequently passed native compilation and finalization and was
installed on the test USB's two Intel kernel-driver override paths plus the
accelerant path. The laptop rebooted successfully. Its initial external pipe
diagnostic read CTL=0x00000008, COLOR=0, STRIDE=0, OFFSET=0, SURF=0, DDB=0 and
WM0=0x00004007. This is a disabled primary plane with no DDB allocation and no
enabled watermark. The required TC2 DDI_IO domain remained off. NanoKVM still
reported VIDevFPS=0 and VIFPS=0. No HDMI output or acceleration was demonstrated.

Build 048, through patch 53, passed native validation and was installed on the
test USB with the explicit raster-test marker enabled. The laptop rebooted and
executed the integrated experimental path. It returned a bounded timeout at
the display-power stage with cleanup=0. SSH remained available; NanoKVM input
counters remained zero. This is a failed output test, not working HDMI.

Linux sequence review then identified an ordering error: port DDI_IO power is
requested after Type-C PHY ownership, PLL programming and port clock enable,
whereas the new manager requested it before those steps. A correction is
being developed and has not yet been physically tested at this checkpoint.

## First physical TMDS output

Build 049 passed Type-C ownership, AUX, PLL and clock stages but still timed
out at DDI_IO. Build 051 captured the accepted request without acknowledgement:
DDI_IO=0x203 (expected masked0x300), TCSS=0x0c, buffer=0xc0,
PLL=0xcc000000, CLKSEL=0x80000000, gate=0x4, AUX=0x303. Cleanup completed.

Patch58 adds the Linux-documented four-lane FIA allocation before PLL setup.
Build052 passed native compilation and artifact validation, was installed on
both override paths of the isolated test USB, and booted on the N100 laptop.
The real raster path completed: stage DDI buffer, status B_OK, cleanup1 meaning
resources remain held for the active output. NanoKVM reported state1,
VIDevFPS51 and VIFPS60. Its authenticated MJPEG stream returned a1920x1080
black frame, which was opened and visually inspected. All external image
planes are intentionally disabled; this demonstrates TMDS video output, not
desktop scanout or GPU acceleration. The internal installed OS was untouched.
Desktop output still requires validated DDB allocation, watermarks and primary
plane programming against the shared framebuffer.
