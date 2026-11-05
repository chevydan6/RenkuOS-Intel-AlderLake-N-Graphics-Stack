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
on that private copy, retaining its previous contents as `.pre-gpu`. Two further
cold boots collect metadata, export `GPU.TGZ` to the image's EFI partition and
request shutdown. Startup moves each previous result directory into a unique
preserved directory, so the Wi-Fi wrapper receives fresh output on every boot.
Fedora then reads the final archive with mtools, verifies the QEMU
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
replacement invokes Astra #1's frozen joint Wi-Fi wrapper synchronously before
export, with a 140-second deadline and a five-second termination grace period.
The existing private association helper/settings remain in the baseline image.
The wrapper and capture script are hash-checked against the reviewed handoff
before preparation. Its exit status is archived without preventing GPU export
or shutdown. VM validation requires the expected absent-radio report; this does
not validate physical association or packet transfer.

Raw images and logs contain private host/network information and stay outside
version control. Snapshot data describes firmware boot memory, not verified
current scanout, GGTT ownership or working GPU acceleration.

`host-preflight.py` records the current Fedora boot ID, internal root, active
drivers, battery and original VM service under the shared locks.
`stage-nanokvm.py` copies only to the GPU-owned inactive staging file and checks
its complete SHA-256; the directory name is retained when updating a candidate.
Its resume option requires the prior writer to have stopped.

`nanokvm-lease.py` is a prepared, syntax-checked cross-boot lock helper. It has
not yet been exercised in a physical test. It holds the existing global lock,
saves fresh LUN/binding values and expires after 30 minutes. It does not switch
media, restore the host, or clear an outage reservation. A durable outage marker
and explicit recovery verification are still required around its use.
