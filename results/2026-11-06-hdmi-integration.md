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
