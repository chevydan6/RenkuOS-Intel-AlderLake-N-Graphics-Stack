<!-- SPDX-License-Identifier: MIT -->
# Physical N100 kernel-service checkpoint

Actual RenkuOS hrev60072+518, four-core Intel N100, host bridge 8086:461c and
GPU 8086:46d1. Native GCC13.3 and matching installed kernel SDK compiled the
driver, client and original Linux execution slices. No VM binaries were loaded.

The exact VM-tested native-006 source archive verified before extraction.
`run-visible.sh` was launched by NanoKVM keyboard in fullscreen Terminal. Its
real build output and all client results were visible. SSH transferred files
and collected this evidence. `native-test.log` includes actual compiler/linker
commands; client logs contain 16 runs × 62 checks, all passing.

These are CPU kernel-service tests: native pages, locks, kernel workers,
timeouts/errors, cleanup and imported Linux allocator/refcount/context code.
The execution preparation object still has unresolved timeline/space-wait
imports. No GPU command, MMIO write or Intel-driver replacement occurred.

`module-lifecycle.log` records real INIT/UNINIT status=0. `cleanup.txt` follows
strict absent-file/device checks. Graphics device listings before/after match.
The physical system remains running its HDMI desktop on external RenkuTest.
Temporary transfer SSH/HTTP servers were stopped, the KVM-side lock was released,
and the attached media path remained unchanged. Original SSH settings were not
replaced. Private authentication material and network logs are outside Git.

Local raw results/binaries:
`/home/dan/i915-vm/evidence/physical-001/` and
`experimental/i915/build/physical-n100-001/`.
Actual visible test screenshot:
`/home/dan/i915-vm/evidence/nanokvm-image-1794558380017140274-0.jpg`.
