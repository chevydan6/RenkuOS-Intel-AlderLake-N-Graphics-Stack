# RenkuOS SSH, MCP and dual-boot preparation

This directory contains preparation code for a private RenkuOS installation.
After the isolated rehearsals, an 8 GiB RenkuOS partition and a separate GRUB
entry were installed on the laptop. Fedora booted successfully afterward with
its existing saved default. After a firmware message involving NanoKVM, the
user reported booting RenkuOS. The boot source and installed system have not
yet been independently confirmed, and SSH has not been found on the network.

The physical Wi-Fi baseline is the verified hrev60072+518 image. The MT7921
driver SHA-256 is
`4404c0ff2f52ec651d9a8d1212d9864df8343ae5fd209899d19400340c725f6d`.
`UserBootscript` starts SSH and, after a delay, invokes the same bounded explicit
Wi-Fi join helper used during the physical test. The private credentials and
helper are inherited from the baseline image; they are not included in Git.
The physical run received five router ping replies while packet capture was in
promiscuous mode. This establishes short local-network operation, not a proven
driver fix, unattended boot reliability, Internet access or long-term stability.

`renku-mcp.cpp` is a small MCP stdio server using the MIT-licensed `json11`
library, pinned to upstream commit `2df9473fb3605980db55ecddf34392a2e832ad35`.
It currently exposes one read-only `system_info` tool. It does not expose a
general command tool; ordinary SSH is available separately. The tested RenkuOS
`user` account reports UID 0. MCP is started by SSH on demand, rather than
listening independently on a network port.

Build natively with the installed RenkuOS C++11 compiler:

```sh
g++ -std=c++11 -O2 -Wall -Wextra -Werror renku-mcp.cpp json11/json11.cpp -o renku-mcp
```

The server expects newline-delimited JSON-RPC on stdin and writes only protocol
messages to stdout. A client can launch it with SSH as a subprocess using the
installed path `/boot/home/config/non-packaged/bin/renku-mcp`. Configure the
client with the actual RenkuOS address and a verified SSH host key after install.

## Validation

- The server passed initialization, discovery, `system_info`, ping, malformed
  request, invalid argument, unknown method and clean shutdown tests on Linux
  and natively in a disposable RenkuOS VM over SSH.
- The official MCP Python SDK 2.3.0 connected to the native server over SSH,
  listed its tool and called `system_info` successfully.
- A private image containing the server, Wi-Fi files and delayed startup passed
  stage and cold-boot VM tests. The Wi-Fi driver and MCP binary hashes matched.
- A separate virtual GPT disk was formatted with an 8 GiB BFS target. RenkuOS
  Installer copied the image to it; the installed binary, authorized key,
  startup and Wi-Fi configuration were present afterward. With the source USB
  image detached, the installed target booted and passed SSH and official MCP
  SDK tests. It used approximately 2.4 GiB after first-boot processing.
- A disposable virtual EFI partition with a GRUB menu entry chainloaded the
  separate RenkuOS EFI loader; target-only boot, SSH and MCP passed again.
  `renkuos-grub.cfg` is a syntax-checked draft for the observed laptop ESP UUID,
  the same entry was later installed on the laptop; its RenkuOS boot path has
  not yet been validated on physical hardware.
- The installed 8 GiB BFS filesystem was copied to the fourth partition of a
  separate sparse virtual disk at the planned laptop offset. The relocated
  system booted, mounted that partition, and passed SSH and official MCP SDK
  checks. This does not validate physical firmware or Wi-Fi behavior.
- GPT headers copied from the laptop were applied to a sparse regular file.
  `gpt-rehearsal.py` shrank its third GPT entry by 8 GiB and added an aligned
  BFS entry, verifying the original partition starts, UUIDs and types. This
  tested GPT metadata only: the file contained no Btrfs filesystem and no
  Btrfs shrink was performed. GNU Parted's noninteractive script mode refused
  the shrink warning; blindly relying on its exit status is unsafe.
- Separately, `btrfs-rehearsal.sh` created a disposable 12 GiB sparse file on
  Fedora, mounted it through a verified loop device, wrote 128 MiB of test data,
  shrank the Btrfs filesystem by 8 GiB, verified the content hash, unmounted it
  and passed `btrfs check --readonly`. The loop device and scratch image were
  removed. This exercises the command on scratch data, not the laptop's live
  filesystem or its particular allocation/fragmentation. The first laptop
  shrink attempt failed for lack of relocation space without changing its size.
  After a backed-up old kernel-build directory was removed from Trash, the
  shrink succeeded and Fedora rebooted successfully.
- `capture-fedora-boot-backup.sh` captured the existing GPT metadata, EFI and
  `/boot` contents, GRUB configuration and fstab. Their SHA-256 checks passed
  after copying to the workstation. This is a boot recovery artifact, not a
  complete backup of the laptop's personal data or running Fedora filesystem.
- A private workstation backup of the root, home and nested machines snapshots
  passed rsync checksum and metadata comparison, apart from synthetic timestamps
  on two confirmed empty Btrfs subvolume placeholders. Boot and EFI archives
  passed SHA-256 checks separately. The active swapfile was omitted and requires
  recreation during a restore. The snapshots were taken at different times.
- The rehearsed RenkuOS BFS image was written to the new partition and its full
  8 GiB SHA-256 read-back matched the source. The physical Wi-Fi, SSH, MCP and
  GRUB-to-RenkuOS path remain unverified.

The target-only boot used a newly generated SSH host key. Installer excludes
the source image's standard host keys; actual installation must establish and
verify the new host key before trusting a network connection. One attempt to
overwrite an existing virtual EFI file with `pyfatfs` failed; the successful
GRUB test used a newly created disposable FAT image. It does not establish that
the laptop's existing EFI partition can safely be edited by that library.

Private raw images, VM disks, SSH keys, Wi-Fi settings and logs are excluded
from this repository. Retain the verified workstation backup and boot recovery
artifacts throughout further disk and boot work.
Physical Wi-Fi, SSH/MCP access, GRUB boot and Fedora recovery must be verified
after installation before claiming the laptop installation is complete.
