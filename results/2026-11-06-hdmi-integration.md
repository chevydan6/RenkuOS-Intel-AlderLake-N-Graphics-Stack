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
DDI_IO=0x203 (expected masked 0x300), TCSS=0x0c, buffer=0xc0,
PLL=0xcc000000, CLKSEL=0x80000000, gate=0x4, AUX=0x303. Cleanup completed.

Patch58 adds the Linux-documented four-lane FIA allocation before PLL setup.
Build052 passed native compilation and artifact validation, was installed on
both override paths of the isolated test USB, and booted on the N100 laptop.
The real raster path completed: stage DDI buffer, status B_OK, cleanup 1 meaning
resources remain held for the active output. NanoKVM reported state 1,
VIDevFPS 51 and VIFPS 60. Its authenticated MJPEG stream returned a 1920×1080
black frame, which was opened and visually inspected. All external image
planes are intentionally disabled; this demonstrates TMDS video output, not
desktop scanout or GPU acceleration. The internal installed OS was untouched.
Desktop output still requires validated DDB allocation, watermarks and primary
plane programming against the shared framebuffer.

The read-only inventory helper subsequently compiled and ran on the laptop
using existing driver areas and the normal accelerant lock. The primary plane
on pipe A occupies DDB blocks [0,2048); every other powered plane/cursor has an
empty allocation. The verified Alder Lake-P/N descriptor has 4096 blocks in four
1024-block slices, and the Linux A+B routing assigns B the free [2048,4096)
range. All four slices are powered, MBUS join/hash are clear, and both active
pipes have matching horizontal/vertical timing totals. Shared framebuffer:
offset 0, stride 7680, RGB32, 1920x1080, mode pixel clock 138780 kHz.

The actual internal PLL registers are enable 0xcc000000, CFGCR0=0x00b555a5,
CFGCR1=0x88, SSC=0x401320ff (SSC enable clear). Clock decoding must include the
display-version 12+ / 38.4 MHz reference fractional correction from Linux; assuming
the nominal mode clock is exact would be incorrect. Desktop watermark planning
remains under development. A later NanoKVM sample still reported VIFPS 60,
VIDropCnt 0 and CSI overflow 0; this is a limited raster test, not a long-term
stability or desktop validation.

Build 054 included the guarded desktop-clone stage and framebuffer lifetime
checks. It compiled natively and booted on the test USB with both test markers.
The desktop stage returned B_OK with zero underrun flags, but NanoKVM captured
vertical color stripes rather than a desktop. A software screenshot showed a
normal Renku desktop, so successful plane enable was not sufficient evidence
of correct output. The test was recorded as failed desktop display.

Read-only live-surface and pipe-format inspection found A/B surface offsets
both zero, but PIPE_MISC A=0x01000000 and B=0x01000080. The documented ADLP
bits 7:5 encode output BPC; B was still set to 12 BPC while its transcoder was
configured for 8 BPC. A scoped pipe-format correction is under development.
No speculative GGTT or framebuffer-backing changes were made.
