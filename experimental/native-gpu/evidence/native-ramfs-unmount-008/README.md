# Physical N100 recovery and RAMFS VM boundary, checkpoint 008

Date: 2026-11-15 (Asia/Tokyo). Branch: `codex/i915-native-adaptation`.

The physical N100 stopped responding after the earlier base-kernel BFS/block
cache panic. The authenticated Govee H5080 outlet read **on**, then confirmed
an OFF transition and an ON transition. NanoKVM showed a RenkuOS desktop with
"Your setting file could not be saved! (Initialization failed)." The KVM
Terminal showed the MediaTek Wi-Fi device present with no IPv4 address and no
received packets. Its saved wireless-network list and keystore file existed;
the desired access point was visible in a scan. A one-session reconnection
using the host's already saved Wi-Fi credential restored `192.168.1.114` and
SSH. The credential was not printed or copied into this repository. It was
not saved on the N100 because its boot filesystem is read-only.

The physical `/boot` BFS volume had about 4 GiB free but rejected creation of
`/boot/home/native-vulkan-compiler-008` with **Read-only file system**. The
known-good owner device `/dev/misc/intel_native_execution` still existed.
The existing Vulkan bundle hash matched its saved checkpoint, and the bounded
ABI probe and all four physical triangle shapes passed after recovery:

```text
Native Vulkan N100 probe: PASS vendor=8086 device=46d1 api=1.0 memory=65536
Native Vulkan N100 triangle: PASS shape=0 hash=28616d2d
Native Vulkan N100 triangle: PASS shape=1 hash=d40eb9c3
Native Vulkan N100 triangle: PASS shape=2 hash=e24aed05
Native Vulkan N100 triangle: PASS shape=3 hash=984ae945
```

No physical compiler-backed bundle was installed or tested. The original
known-good bundle and native GPU owner were not changed. The boot filesystem's
read-only state, not GPU rendering, blocks installing new userspace artifacts.

## Disposable VM RAMFS experiment

To avoid writing the affected physical BFS volume, an empty mountpoint in the
disposable Haiku VM was used to test `mount -t ramfs`. Mount succeeded and
`df` identified a writable RAMFS. `unmount` then triggered a base-kernel VFS
assertion at `src/system/kernel/fs/vfs.cpp:147` (`vnodes.IsEmpty()`) in the
`unmount` thread. [The saved KVM screenshot](vfs-unmount-panic.png) has SHA-256
`20f17f92170ef0e24a6986502ee02270f8ec139f9c9c815d93c676aa6f3bebce`.
Several VM scratch files failed their previous hashes after reset, so the VM
was restored from `native-compiler-api-001`; SSH and the original compiler
library hash were checked afterward. No RAMFS mount was attempted on the
physical N100. No base-kernel source was changed.

The compiler-backed userspace binaries remain saved on the host in the
isolated `haiku-compiler-artifacts-008` staging directory. Physical testing
requires a verified writable volume or a separately justified recovery path.

## Restored VM validation

After rollback, the source and preserved compiler libraries were copied into
`/boot/home/test-work/compiler-pipeline-restored-008` on the VM. The current
compiler-backed Vulkan library was rebuilt, and both probes passed:

```text
compiler pipeline probe: matched hardware-tested fragment; rejected altered input and short buffer
Vulkan API probe: rejected malformed shader module; compiled accepted pipeline; rejected altered pipeline
```

The rebuilt VM `libvulkan.so` SHA-256 is
`083dbf5605d66f43154d20081288bc6882e1880da426841f92451438221a9f06`.
The physical N100's original `api_probe` and four triangle shapes were run
again after the VM test and returned the same pass lines and hashes above.
This validates the current physical stack, but the new compiler-backed build
has not run on the physical GPU.

## Read-only filesystem diagnosis

The physical boot volume is BFS on `/dev/disk/usb/0/0/1`. `df -h /boot`
reported 4.0 GiB free. A subsequent `checkfs --check-only /boot` completed
after checking 118,968 nodes. It reported `size (inode = 4455), invalid
b+tree` and `configure-2.log (inode = 1579760), could not be opened`.
Its final allocation summary was zero blocks unallocated, already set, or
freeable. The command returned status 0 despite those diagnostics, so that
status is not evidence of a clean filesystem. This read-only scan did not
attempt a repair. The exact cause of the read-only mount is still unproven;
the reported metadata errors warrant preserving the volume before repair.

## Backing-media capacity

The physical boot device identifies as the NanoKVM USB gadget (`3346:1009`).
On the NanoKVM, the active mass-storage LUN names
`/root/renku-gpu-test-004/disk.img`; its `ro` setting is `0`. The image's
logical size is 9,221,177,344 bytes, while `du` reports about 2.7 GiB of
allocated storage. The NanoKVM root filesystem reports 7.6 GiB total and
**zero available bytes**. Its `dmesg` did not show a matching write error in
the bounded search performed here. A full backing filesystem can prevent
growth of a sparse image despite the guest BFS volume reporting 4.0 GiB free.
This is a plausible additional cause of the read-only guest state, not proof
of which failed write first. No NanoKVM media setting or file was changed.

An initial read-only partition image through the guest USB path was stopped
after it transferred too slowly; its host file is marked `.partial.img.zst`.
A direct read-only copy of the active NanoKVM image completed at
`/home/dan/ai/backups/renkuos-n100/nanokvm-disk-2026-11-15.img`, with the
expected logical size of 9,221,177,344 bytes. Its host SHA-256 is
`92d939219817af21e4277dd825fde5ea54bdac2bb146864bfb71e0deada7826c`.
The active NanoKVM source image produced the same SHA-256 after a complete
read-only hash. The host copy is therefore a verified recovery backup for the
source image as read on 2026-11-15.

The NanoKVM also mounts `/data` from `/dev/mmcblk0p3` with about 111.4 GiB
free, but that exFAT volume is mounted read-only. An inactive Fedora rescue
ISO was copied to the host at
`/home/dan/ai/backups/renkuos-n100/nanokvm-inactive/Fedora-rescue.iso`.
Both copies had SHA-256
`bd285201494dd0ba09b54d05ac707de1401668b8512a573edb5922dcf9d7067e`
and size 1,217,329,152 bytes. After checking that the active LUN still
named `/root/renku-gpu-test-004/disk.img`, the inactive ISO was removed
under the shared physical-test lock. NanoKVM root then reported 833.7 MiB
available (89% used). The active LUN path and media settings were not changed.

## Offline repair rehearsal

A separate host clone of the verified image was attached as a second USB disk
to the disposable RenkuOS VM. Mounted read-only, it reproduced both physical
`checkfs --check-only` errors. Running repair-mode `checkfs` against that
read-only mount panicked **the disposable VM only** with
`block_cache_make_writable: tried to make block writable on a read-only cache`.
After restarting the VM, a fresh clone was mounted writable. Repair-mode
`checkfs` completed with 118,974 nodes checked and 20,152 blocks that could be
freed. A second `checkfs --check-only` reported 118,974 nodes checked, no
invalid B+ tree or unreadable inode messages, and zero blocks that could be
freed. The known-good native Vulkan and RCS bundle directories remained
present. The repaired clone was unmounted and detached from the VM.

The final detached repaired image SHA-256 is
`6ea4bdd0092b4b3691ac59494c343b4289d2512f6a221687c0bbc8bb84fe06ce`.
It differs from the original in 8,215 4 KiB blocks (33,648,640 bytes).
An `rsync --inplace --no-whole-file --checksum` rehearsal on another host
copy transferred 177,452,352 literal bytes and produced the exact repaired
image SHA-256. The rehearsal copy was mounted read-only in the disposable VM
after the repair clone's unmount flush; `checkfs --check-only` again checked
118,974 nodes with no invalid inode messages and zero blocks to free. Its
SHA-256 was unchanged after the read-only mount and unmount.

With the physical N100 shut down and its Govee H5080 outlet confirmed off,
the same `rsync` command updated the active NanoKVM image in place under the
shared physical-test lease. It exited 0 after sending 177,356,328 literal
bytes; the active LUN path remained unchanged, and NanoKVM root reported
about 832.4 MiB free. A full post-transfer SHA-256 of the active image matched
the detached repaired clone exactly:
`6ea4bdd0092b4b3691ac59494c343b4289d2512f6a221687c0bbc8bb84fe06ce`.
The LUN still named the same image with `ro=0` and `cdrom=0`.

After the outlet was restored to on, the N100 booted the expected
`hrev60072+518` kernel and mounted `/boot` writable (`QAM-P-W`). The installed
Vulkan [API probe](postboot-controls.log) passed. Each of four known-good triangle shapes passed 16
times, preserving full-image hashes `28616d2d`, `d40eb9c3`, `e24aed05`, and
`984ae945` in the [64-draw log](postboot-triangle-64.log). The version 7 RCS public vertex/shader draw also passed again with
sequence `71/71`, `777/777` matching pixels, hash `28616d2d`, and no GPU fault.
The physical `checkfs --check-only` reported 118,974 nodes and no invalid
inode messages. A first scan wrote its own output log onto `/boot` and
reported 32 blocks that could be freed; a second scan with output captured on
the host [reported zero such blocks](postboot-check-summary.log). No RenkuOS kernel source or binary was
changed.
