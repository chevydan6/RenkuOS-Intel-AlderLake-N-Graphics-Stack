# External USB boot diagnostic

These scripts prepare a private copy of the preserved RenkuOS baseline on
Fedora. They install only the read-only `boot_graphics` diagnostic and inventory
tool; they do not install the experimental Intel/GART patches.

Run `prepare-usb.py --output ~/renkuos-development/testing/gpu-physical-NNN`
through the shared coordinator with `host-config`, `haiku-vm`, `nanokvm` and
`build` reserved for the full transaction. The script expects the verified kit,
collector and guest preparation script under `builds/gpu-004`, plus the existing
guest SSH configuration and a forwarded agent. It refuses an existing output
directory and verifies the baseline image hash before and after preparation.

The first QEMU USB/UEFI boot installs the diagnostic and replaces UserBootscript
on that private copy, retaining its previous contents as `.pre-gpu`. The second
cold boot collects metadata, exports `GPU.TGZ` to the image's EFI partition and
requests shutdown. Fedora then reads the archive with mtools, verifies the QEMU
PCI control and repeated snapshots, and records the final image hash. The
original guest, baseline image and NanoKVM media are not changed by this script.

This workflow does not itself authorize or perform a physical reboot. Before
using the candidate, coordinate the hardware window and recovery, preserve the
current NanoKVM LUN/binding, and verify a staged copy's full hash. Reserve the
operation across the host outage and retain ownership until Fedora and the
original guest are restored. Permanent boot order and internal disks must remain
unchanged.

The prepared candidate contains results from its VM validation. Physical
evidence must be distinguished from those results: require the exact physical
PCI identities and successful collection, and preserve before/after provenance.
Do not accept an unchanged VM archive as evidence of a hardware run. The startup
replacement does not start the baseline's private Wi-Fi association procedure;
a joint Wi-Fi boot requires a separately coordinated and tested startup change.

Raw images and logs contain private host/network information and stay outside
version control. Snapshot data describes firmware boot memory, not verified
current scanout, GGTT ownership or working GPU acceleration.
