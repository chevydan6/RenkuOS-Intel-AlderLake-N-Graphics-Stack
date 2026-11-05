# USB/UEFI diagnostic preparation

`gpu-physical-003` passed both guest preparation and automatic cold-boot capture
in QEMU. Both guests requested power-off and QEMU exited zero. The EFI archive
was recovered with mtools and validated after shutdown. Coordinator unit
`astra-astra-gpu-c5a8bfdd2a1e.service` completed successfully at
2026-11-05 04:53:04 UTC.

- Source image SHA-256:
  `0a8b87be6986acb3bf39c9a4f21431b1e75ebf56eb5a41fd0e51c479877a62a0`
- Prepared image SHA-256:
  `215ca882b282dd992f151752b852a26932ea3dc140680a9b6287e0c17a16cb08`
- Image size: 690,814,976 bytes.
- Collection status: zero; QEMU PCI control: expected non-target status 3.
- All five automount flags remained disabled; three boot descriptors and three
  active framebuffer metadata snapshots were internally consistent.

OVMF selected 4096 × 2160, depth 32, stride 16384. The boot physical base was
`0x80000000` and visible storage 35,389,440 bytes. The active driver reported
`framebuffer.accelerant` and matching dimensions. This is emulated framebuffer
evidence, not an Intel N100 resolution or acceleration result.

Two preceding preparation failures are retained. Run 001 assigned xHCI to the
fixed PCI address expected by the VGA control; explicit QEMU PCI addresses fixed
that mismatch. Run 002 shut down cleanly but did not export its archive: Haiku's
`df` does not support `-v`, and its FAT module is named `fat`, not `dos`. The
collector was corrected and run 003 verified the complete export path. Only the
failed run 001's generated raw copy was reclaimed after checking that its VM
had stopped; the source image and failure records were preserved.

At 04:56 UTC NanoKVM SSH, authenticated API and a fresh MJPEG frame worked.
Video showed Fedora and the original Haiku VM. Its USB gadget was configured
and its LUN still referenced `/dev/mmcblk0p3`. No physical reboot, media switch,
Intel driver execution or hardware acceleration was performed by these checks.

The Wi-Fi agent requested a joint capture under GPU boot ownership. Its script
and association startup must be integrated and cold-tested before using a joint
candidate; the current GPU-only candidate is not evidence of that integration.
