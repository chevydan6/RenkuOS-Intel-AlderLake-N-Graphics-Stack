# Native graphics inventory validation

The C++ diagnostic built with GCC 13.3.0, optimization, warnings-as-errors,
exceptions disabled and RTTI disabled in RenkuOS hrev60072+518 x86_64. It uses
interface headers exported from the pinned source, without installing a driver.

In `gpu-native-019` / `boot-inventory-002`:

- Invalid CLI input returned 2 before any device access.
- Fixed PCI reads reported bridge `8086:1237` and GPU `1234:1111`; the diagnostic
  returned 3 before reading Intel-specific registers.
- Three firmware-display metadata captures matched: VESA, 1280×800, stride
  5120 bytes. Each explicitly reported that the physical address and GGTT
  translation were unavailable/unverified.
- All 20 source payload hashes matched. The artifact's ELF64/x86_64 header and
  hash were checked after retrieval.
- The guest shut down cleanly; QEMU exited 0. No GPU workload or driver install
  occurred. The unchanged twelve-patch driver series was not rebuilt in this
  diagnostic-only run; its separate previous native validation still applies.

The 30,384-byte executable has SHA-256
`689e7cdf22eb67c416e7a8d894e6b458d0422d0678f9bf4d803d384fc479f37c`.
The bundle manifest hash is
`28a3ee1576d923a81f96021282d7d3a35f8bb59e5d4193f2e6563fc9c0859727`.
Private evidence is under `logs/renku-vm-019/validation`, including source
checks, compiler/OS identity, repeated output, ELF information and the binary.
The coordinator unit was `astra-astra-gpu-08b3a52199cf.service`: 89.025 seconds
wall time, 64.122 CPU seconds, peak 1.3 GiB across the transaction.

## Access failures and recovery

Runs 017 and 018 never reached the build. Their private guests were stopped
through QMP and their evidence retained. Added per-attempt SSH logs exposed
authentication failures in run 018: the temporary forwarded agent contained
the Fedora identity but lacked the guest's authorized identity. Loading the
existing guest identity with a short expiry enabled run 019; no guest network
or host configuration change was needed. The runner can now accept a bounded
readiness-attempt count (1–40), defaulting to its original 20.

Run 019 cloned our cleanly stopped run-016 disk. A QMP screenshot confirmed the
desktop in run 018. An attempted console `ifconfig` in run 019 arrived during
the boot splash and produced no useful result; it is retained in the private
console-action log. No console network-configuration command was sent.

## Physical-address limitation

An initial unexecuted prototype queried a framebuffer clone through
`POKE_GET_PHYSICAL_ADDRESS`. Source review found that `get_memory_map_etc`
panics on an unmapped page, while a graphics mode change can invalidate the
clone. This path was removed before any native execution. The final diagnostic
clones only metadata read-only and never clones or accesses pixels. Both
existing firmware accelerants return NULL for `frame_buffer_dma`, so that hook
does not supply a safer physical-address query.

This does not establish Intel attachment, native display initialization,
framebuffer ownership, GGTT safety or acceleration. The next mapping diagnostic
needs a kernel-side query synchronized with mapping lifetime, or another
verified interface. Reproduction and scope are in `diagnostics/README.md`.
