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
