# Native N100 boot diagnostic

The joint GPU/Wi-Fi diagnostic ran on the physical N100 in RenkuOS
hrev60072+518. The GPU inventory returned success and identified bridge
`8086:461c` and GPU `8086:46d1`. This was the existing framebuffer path plus
the private read-only boot diagnostic, not the experimental Intel/GART series.
No GPU command execution or acceleration is claimed.

| Observation | Value |
| --- | --- |
| Firmware boot framebuffer base | `0xd0000000` |
| Dimensions / depth | 1920×1080 / 32 bits |
| Stride / visible bytes | 7680 / 8,294,400 |
| Existing accelerant | `framebuffer.accelerant` |
| Repeated metadata | Three identical boot snapshots and three identical display snapshots |
| Built-in screen | RenkuOS desktop reported by the attending user |
| HDMI / NanoKVM video | Unavailable during native run |
| Automount | All five flags disabled |
| Wi-Fi router test | Five replies from five requests, zero packet loss |

Boot metadata and active framebuffer dimensions agree. The physical boot base
equals the PCI aperture base, but the GGTT entries and underlying memory
ownership remain unknown. These observations do not authorize treating the
firmware framebuffer or protected stolen memory as allocatable space.

The fixed PCI reads also matched the earlier Linux observations: GGC `0xfec1`,
BDSM `0x7c800001`, BGSM `0x7c000001`, BAR0 low `0xbf000004`, BAR2 low
`0xd000000c`; both BAR high halves were zero. The known diagnostic binary
hashes matched. Collection, association-wrapper and Wi-Fi-capture statuses
were zero. The bounded router ping is connectivity evidence, not a claim of
Internet access or long-term Wi-Fi stability; Astra #1 owns its further analysis.

The image was first validated with two consecutive cold boots in QEMU. Before
physical use its complete SHA-256 matched on NanoKVM:
`a41058166c10e7b67bb6efec1fcd215d4ab15236eae71568abc4377bfdc0f672`.
The native archive, exported to the image's EFI partition at approximately
06:14 UTC, differs from the VM archive and contains the physical PCI identities.
Its SHA-256 is
`9c827506d3c52493c73d15ba40fea2f7714333e8a5e1e859573b27ec5fdf8df8`.
All archive members were read successfully. Private logs, packets, images and
network settings remain excluded from Git.

## Recovery observations

The original Haiku hrev59866+79 development VM panicked in TCP endpoint cleanup
during the pre-test SSH shutdown attempt. Panic/serial evidence and a reflink
copy of its disk were preserved. A read-only QCOW structure check passed; this
does not establish BFS consistency. Restart restored its desktop, and a virtual
ACPI power-button request then shut it down successfully with QEMU exit zero.

During the physical run NanoKVM reported USB detached even while the user
reported a RenkuOS desktop. Detached USB therefore was not accepted as sufficient
proof of power-off. The native archive was recovered from a read-only copy of
the EFI region; no live target filesystem was mounted by the host. After the
user explicitly confirmed complete power-off, the saved LUN file, writable/CD
flags and UDC binding were restored exactly. Permanent boot order and internal
disk contents were not intentionally modified. The user subsequently reported
Fedora back. Recovery checks subsequently passed: a fresh host boot ID, the
same internal eMMC root, active i915/mt7921e drivers, interface-bound Fedora
Wi-Fi HTTPS, and the original Haiku guest's SSH and NAT HTTPS. Its serial log
contained no new panic. Exact media restoration was rechecked, the cross-boot
lock was released, and the durable outage was archived as restored.
Recovery units `astra-astra-gpu-ad4f96c0787e.service` and
`astra-astra-gpu-700231e43537.service` both exited successfully.

The next GPU milestone is to establish firmware GGTT/backing-memory ownership
without disturbing the working framebuffer. The aperture address alone is not
enough to install the experimental GART path or submit a blit. HDMI bring-up,
accelerated desktop operations and modern command submission remain open.
