<!-- SPDX-License-Identifier: MIT -->
# Disposable VM kernel owner and RAM-clone probe

Status: Astra reviewed and executed this fixture in native build086. This is a
standalone **test driver** and client, not part of the production source tree.
The actual owner module becomes `B_KEEP_LOADED` when successfully acquired, so
run only in a new disposable RenkuOS VM snapshot with the reviewed matching owner
module and the canonical VM/host/build locks. Never install in a physical target.
No production test hook, PCI match bypass, GPU register access, or GPU workload.

The test driver's per-open cookie owns a wired RAM-backed kernel area created
with the same `B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA`
protection flags as the owner mapping. The `GET` ioctl calls actual `get_module`
and `put_module` on the retained owner and requests `begin_display(255,255,255)`.
Device/function 255 cannot represent a PCI device/function, so a correct owner
must return `B_NOT_SUPPORTED` before creating an aperture or mapping any BAR.
The output lease and resource bytes must stay unchanged. The client checks this,
then clones the RAM area, checks the kernel-written sentinel, writes a second
sentinel, asks the kernel to confirm it, closes the device and checks that its
existing clone survives original-area deletion. It deletes the clone and exits.

## Review and build handoff

Astra should review the source and adapt a disposable native SDK build using the
existing GPU bundle's Makefile kernel settings (`_KERNEL_MODE`, private kernel and
graphics include paths, `-fno-exceptions -fno-rtti`, `-shared -nostdlib` and
`_KERNEL_` linkage). Build the userland client with the same SDK. Do not install
anything until the source, binary symbols, dependencies and exact disposable VM
paths are reviewed. Install the test add-on only into the copied guest's
nonpersistent test environment using the existing VM runner's supported workflow;
arrange its devfs node at `/dev/misc/terra_owner_probe`. Do not replace the
production graphics driver or the host's system files.

Record source/binary SHA-256, kernel and compiler identity, module dependency and
installation paths, module-load status, expected/actual ioctl statuses, untouched
output evidence, area IDs, clone results, userland exit, kernel log and clean VM
shutdown. A native run validates actual module-manager/admission and RAM area
semantics only. It does not validate an N100 PCI device, MMIO cache attributes,
real IRQs, GPU execution, physical display, hardware recovery or area clones
across legacy driver unload. The owner's real PCI and GART module dependencies
may initialize; verify they do not access/map a GPU on the chosen QEMU guest.

The client's 15-second alarm bounds userland waiting only; it cannot interrupt an
uninterruptible kernel call. Use the VM runner's external watchdog and QMP cleanup.
On any unexpected success selecting the invalid BDF, stop immediately, preserve
the VM and logs, and treat as a failure. `B_KEEP_LOADED` means a successful module
lookup and its dependencies persist until the disposable guest shuts down; do not
expect `put_module` to unload it. The test device's final close deletes only its
own RAM area and frees its cookie. Existing user clones intentionally keep their
RAM backing alive until `delete_area`.

## Astra: native086 kernel owner/RAM probe — 2026-11-07

Implemented: isolated test driver/client and auxiliary Makefile; production0085
candidate unchanged. Native compilation, component fixtures and finalizer passed.
Actual owner kernel module loaded; impossible PCI BDF rejected with unchanged
outputs. User RAM clone survived original kernel area deletion, checked explicitly
with get_area_info. Disposable VM shut down cleanly. Owner SHA-256 matches the
finalized artifact: 592fca23f7b96b289ae9e602a3d8f08d572dbd157285255603946d33a4975b10.
Evidence: /home/dan/renkuos-development/installation/private/gpu-hdmi-build-064/
(native.log, probe-result.json, probe-hashes.json, kernel-probe.log, finalize.log).
Attempt063 client passed but runner failed decoding binary syslog; it is incomplete.
Fresh repeat064 captured bytes and completed cleanly. The finalizer's component
scope excludes module loading; separate probe evidence establishes this result.
No new host simulation. No N100 BAR, real IRQ, physical test or GPU workload.
IRQ acquisition/unwind defects, desktop/MMIO clone compatibility, retained-owner
reopen, borrowed-buffer ownership and physical recovery remain unresolved.
Terra independent evidence review requested. Next: actual IRQ setup fault tests
and explicit resource ownership; no production interrupt fix claimed yet.

Build with the bundle Makefile and `-f tests/native-owner-probe/Makefile`, targeting
`build/terra_owner_probe` and `build/terra_owner_probe_client`.
