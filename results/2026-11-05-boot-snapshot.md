# Read-only boot descriptor: native kernel validation

The private `boot_graphics` diagnostic loaded in RenkuOS hrev60072+518 x86_64
and passed **82 native kernel-interface checks**. Its numeric descriptor helper
also passed **34 checks**, both natively and under host address/undefined-behavior
sanitizers. This is real diagnostic kernel execution in QEMU, not Intel driver
execution or GPU acceleration.

The kernel supplies the immutable firmware boot item. The diagnostic exports
only a 48-byte numeric snapshot, with a version, initialized reserved fields,
physical base, visible byte count, dimensions, depth and stride. It has no PCI,
MMIO, pixel-access, allocation or command-submission path. It never translates
a live framebuffer clone through `get_memory_map`.

Run `gpu-native-022` / `boot-inventory-005` reported:

| Boot descriptor field | Observed |
| --- | --- |
| Physical base | `0x00000000fd000000` |
| Visible bytes (stride × height) | 4,096,000 |
| Dimensions | 1280 × 800 |
| Depth / stride | 32 bits / 5120 bytes |
| Current scanout verified | false |
| GGTT translation verified | false |

The test checks the expected std-VGA snapshot, ABI fields, four invalid lengths
with unchanged output, unknown operations, null and wrapping kernel-address
outputs, refused stream reads/writes, 64 repeated opens and queries, and
unprivileged open plus an inherited descriptor after dropping UID. Malformed
requests returned errors without a kernel panic. The native build used GCC
13.3.0 with exceptions/RTTI disabled and warnings-as-errors. The diagnostic
module links only against `_KERNEL_` and exports the expected legacy hooks.

Source/interface baseline: `4799bb5a188c4bb6bbef967287640cc21a061a0d`.
All 32 payload hashes matched. Manifest SHA-256:
`70a0e943e44c8fa97b25f248aa1a2c6b7b0f14074f2fad2e1e681884c21f93c0`.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `boot_graphics` | 15,216 | `806999f1ec810b637a72d28055848bf3b8828f9f5d12a8cb9b4e4a28827959a5` |
| `boot-graphics-inventory` | 32,680 | `6e4c20547b3f02e34f2fc6fdbcc8a52b4f97bfe4491c351f08412cc2a807e937` |

The coordinator unit was `astra-astra-gpu-4cd52493234c.service`: 58.589 seconds
wall time, 64.273 CPU seconds, peak 1.4 GiB. Guest installation used atomic
publication of a staged file and a bounded wait for the legacy driver watcher.
It refused existing files/symlinks. Cleanup reported success, guest shutdown
was orderly, and QEMU exited 0. Private evidence and binaries are in
`logs/renku-vm-022/validation`; `tools/finalize-boot-inventory.py` verifies them.

## Failures retained

- Run 020 compiled/linked successfully but opened the device before the
  asynchronous driver watcher published it. Its files were removed and the
  guest shut down cleanly. The next bundle added atomic staging and a bounded
  publication wait.
- Run 021 reached all 82 checks; one assertion expected `B_DEV_INVALID_IOCTL`
  directly. The kernel's `src/system/kernel/fs/fd.cpp` translates that status
  to POSIX `ENOTTY`. Correcting the userspace expectation made run 022 pass;
  the driver implementation was unchanged. Both failure logs are retained.

The Intel twelve-patch series remains uninstalled. No physical laptop boot or
graphics configuration changed. This snapshot cannot identify current scanout
after a mode change, physical memory ownership, or the GGTT entries backing a
PCI aperture. A native N100 observation still needs a controlled hardware
testing window and verified recovery.
