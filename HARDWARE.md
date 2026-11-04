# Hardware reconnaissance — 2026-11-04

Collected before implementation over the existing verified SSH routes.

| Property | Observed |
| --- | --- |
| Local orchestration workstation | Ubuntu 24.04.5, AMD Ryzen AI 9 HX 370; not the target |
| Target host | Fedora Linux 44 Xfce, x86_64 |
| CPU | Intel N100, family 6 model 190 stepping 0, four cores |
| Host bridge | 00:00.0, 8086:461c |
| GPU | 00:02.0, 8086:46d1, revision 00, subsystem 8086:46d1 |
| Architecture | Alder Lake-N, Xe-LP; Haiku generation 12; Linux display version 13.00 stepping D0 |
| Kernel | 7.0.10-201.fc44.x86_64 |
| Active GPU driver | i915; xe is also loaded but does not own this device |
| Mesa | 26.0.7-2.fc44; glxinfo reports accelerated ADL-N, OpenGL core 4.6 |
| Physical memory | 3.7 GiB total; about 1.5 GiB available during first probe |
| Swap | 9.7 GiB; about 284 MiB used |
| Display connections | eDP-1 and HDMI-A-1 connected; eDP reports 1920x1080; DP-1 disconnected |
| GPU ownership | boot_vga=1; i915drmfb primary framebuffer |
| IOMMU | intel_iommu=on; translated domain; interrupt remapping enabled |
| GPU group | group 0, sole member 0000:00:02.0 |
| Reserved region | DMAR reports RMRR 0x7c000000–0x803fffff; assignment implications need investigation |
| QEMU | 10.2.2, KVM enabled, host CPU, two vCPUs, 1024 MiB RAM |
| VM devices | i440fx/PIIX, IDE qcow2, e1000, std VGA, USB tablet |
| Guest | Haiku R1/beta6 hrev59866+79, x86_64, GCC 13.3.0 |
| Guest graphics | 1234:1111, /dev/graphics/vesa; no Intel GPU passthrough |
| Guest memory | ~1 GiB total, ~814 MiB free at collection |

The host journal reports loaded DMC `i915/adlp_dmc.bin` v2.20, GuC
`i915/tgl_guc_70.bin` v70.49.4, HuC `i915/tgl_huc.bin` v7.9.3;
GuC submission, SLPC and RC enabled, HuC authenticated. These describe the
Linux configuration, not proof that every firmware component is mandatory for
a minimal Haiku display or blitter path. DMC display power, GuC scheduling/power,
and HuC media roles must be evaluated separately. Firmware is not redistributed.

GPU isolation alone does not prove assignability, reset support, usable OpRegion,
stolen-memory access, guest host-bridge matching, or display recovery. Passing
through only the GPU would still leave an emulated host bridge, which matters
to Haiku's current paired GART lookup. No VFIO rebind was attempted.

NanoKVM has an existing management path through USB networking. Wi-Fi records
verify SSH/video/keyboard, but power-cycle/BIOS recovery is not verified. GPU
reset or display loss can also remove NanoKVM video. Require a verified recovery
procedure and explicit authorization before changing host graphics or boot.
