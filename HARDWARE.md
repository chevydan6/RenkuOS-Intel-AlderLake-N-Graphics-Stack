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

## Read-only PCI configuration, 2026-11-05 JST

Under the authoritative host-config lock, authenticated read-only `setpci`
queries confirmed bridge `8086:461c` and display `8086:46d1` both report GGC
`0xfec1`. Intel's documented fields decode to **60 MiB DSM** (GMS `0xfe`) and
**8 MiB GGTT** (GGMS `3`). The bridge and GPU mirror agree on this laptop.

BAR0 low/high: `bf000004` / `00000000`; register base `0xbf000000`.
BAR2 low/high: `d000000c` / `00000000`; aperture base `0xd0000000`.
With the documented 8 MiB table offset, the GGTT window begins at `0xbf800000`.
That final address is calculated, not a RenkuOS mapping validation. These
commands contained no PCI writes, and Fedora continued using i915.

Authentication was supplied through an ephemeral, mode-0600 FIFO in a private
directory and removed after the command. No credential is in this repository,
command scripts or collected test logs. Evidence is retained under the private
`logs/renku-vm-005/pci-config-20261105.txt`.

## Stolen-memory bases, 2026-11-05 01:42 UTC

Under exclusive host-config/haiku-vm/build locks, a fixed read-only collector
verified the same bridge/GPU IDs and i915 ownership. Bridge offsets B0/B4 read
`7c800001` / `7c000001`; GPU C0/C4 read `7c800001` / `00000000`.
The bases decode to:

| Region | Inclusive physical range | Size |
| --- | --- | --- |
| Graphics data stolen memory | `0x7c800000–0x803fffff` | 60 MiB |
| GTT stolen memory | `0x7c000000–0x7c7fffff` | 8 MiB |

The GPU mirror agrees with the bridge, and Linux `/proc/iomem` independently
reports the entire data-stolen range. Together the ranges match the previously
observed RMRR extent. This agreement does not establish passthrough support.
Intel documents the bridge
[BDSM](https://edc.intel.com/content/www/us/en/design/publications/12th-generation-core-processor-datasheet-volume-2-of-2/base-data-of-stolen-memory-bdsm-0-0-0-pci-offset-b0/)
and [BGSM](https://edc.intel.com/content/www/us/en/design/publications/12th-generation-core-processor-datasheet-volume-2-of-2/base-of-gtt-stolen-memory-bgsm-0-0-0-pci-offset-b4/)
fields; Linux's
[Gen11+ BSM definitions](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/include/drm/intel/i915_drm.h)
identify the GPU's two-dword base register.

No protected-range MMIO descriptor or boot scanout PTE was read. A stolen
physical base does not prove that the corresponding GGTT prefix is identity
mapped, firmware-free or safe for allocation. The collector performs no PCI
writes, raw MMIO mappings, debugfs mounts or driver/configuration changes.
Evidence is private in `logs/stolen-inventory-20261105`; the credential channel
was an ephemeral FIFO and was removed after use.
