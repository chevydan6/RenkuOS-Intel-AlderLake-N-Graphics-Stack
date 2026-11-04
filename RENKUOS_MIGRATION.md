# Astra High #2 — RenkuOS graphics migration assessment

Decision: **use RenkuOS as the primary source-development target**, retaining
Haiku as a preserved baseline. Runtime compatibility is not yet certified.
Inspection date: 2026-11-04. RenkuOS pin:
`4799bb5a188c4bb6bbef967287640cc21a061a0d` (main, committed 2026-11-03 23:53 UTC).

## Project, policy and build viability

[RenkuOS/Source](https://github.com/RenkuOS/Source) is active, public and not
archived. It describes an independent Haiku-derived OS. Recent main commits,
merged PRs and successful PR-gate runs were observed; this is evidence of active
maintenance, not a stability guarantee. Its root ReadMe.Compiling.md still says
Haiku and links upstream releases, so prefer its actual workflow and pin files.

[AGENTS.md](https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/AGENTS.md)
explicitly permits machine assistance. Read together with
[DECISIONS](https://github.com/RenkuOS/PlanAndGovernance/blob/main/DECISIONS.md),
[CONTRIBUTING](https://github.com/RenkuOS/PlanAndGovernance/blob/main/CONTRIBUTING.md),
[REVIEW](https://github.com/RenkuOS/PlanAndGovernance/blob/main/REVIEW.md), and
[engineering charter](https://github.com/RenkuOS/PlanAndGovernance/blob/main/engineering-charter.md):
new code is MIT, inherited licenses remain; MIT-compatible BSD/ISC are accepted
by D3. Other imports need per-case review. Preserve notices and use paired
Ported-from/Ported-from-license trailers, Signed-off-by and Assisted-by where
applicable. One logical fix per review; no changes pushed to upstream main.

Build remains Jam/configure, x86_64-unknown-haiku tools, Haiku-style kernel and
accelerant add-ons. Current workflows build x86_64 and advisory x86_gcc2h; older
policy text saying 32-bit is absent is stale relative to the workflow. The
`.github/toolchain.pin` file uses **master/main branches**, not immutable commits;
a reproducible local environment must additionally pin the resolved toolchain
commit or container digest. Avoid rebuilding the full toolchain on a 4 GiB host
while a VM or Wi-Fi tests are active.

The nightly release advertises `RenkuOS-x86_64-20261103-build00071.iso`,
690814976 bytes, SHA-256
`ef0dfaa9aaab5243c8fffbbe609c76e48bf87c4da02bb7f971fb61c608108b71`.
The mutable nightly release's original publication timestamp is not its current
asset build date. This asset is a boot candidate, not proof of the main commit
or a matching development package. Astra High #1 owns separate VM provisioning;
the working Haiku guest and Fedora boot remain preserved.

## Verified interface and source comparison

Compared Git blobs against Haiku `4f1074d5e14f59e0f5e5c4938c8a50d1290fa59c`:

| Area | Result / migration consequence |
| --- | --- |
| Accelerant.h | Identical: same hook ABI at these pins |
| PCI.h, KernelExport.h | Identical: same declarations at these pins |
| Intel shared header | Identical: same device classification and shared structures |
| Intel hooks.cpp, engine.cpp | Identical: unsupported legacy hook problem remains |
| Local AccelerantHWInterface.cpp | Identical: no legacy 2D-hook desktop integration |
| Drivers.h | RenkuOS lacks newer B_GET_DEVICE_SERIAL_NUMBER; existing preceding values unaffected |
| Intel GART and PCI table | RenkuOS adds Haswell server and ValleyView enablement, VLV/CHV GGC/stolen-memory fixes; preserve them |
| N100 pair | GPU 46d1 recognized, bridge 461c + GPU 46d1 still absent from GART |
| ArchitectureRules | Diverged link/debug/compiler rules; use RenkuOS's actual build environment |
| Network compat | RenkuOS already contains mediatekwifi/mwx and compatibility changes; Wi-Fi agent notified |

This establishes compatibility of the inspected source interfaces. It does not
establish full binary compatibility, identical kernel memory semantics, package
compatibility or safe loading of Haiku-built binaries into RenkuOS. Rebuild for
the target and test matching kernel/accelerant components.

## Existing graphics work to account for

- [PR 149](https://github.com/RenkuOS/Source/pull/149) adds Haswell 041a with bridge
  0c08; its paired-GART failure reinforces the N100 finding. Its hardware claims
  concern Haswell, not our N100.
- [PR 48](https://github.com/RenkuOS/Source/pull/48) enables ValleyView; the main
  tree has corresponding discovery and memory-sizing differences.
- [PR 98](https://github.com/RenkuOS/Source/pull/98) is **open and draft**, head
  `544c5b738194a5b228e8a7951fcdba8af50f87d6`: experimental **Gen9** render node,
  GEM, PPGTT, execlists, sync objects and tests. Its author reports Haiku hardware
  results for 8086:191e, explicitly not RenkuOS or Gen12 validation. It also records
  Genode-derived AGPL material with linking exception and pending license review.
  No code from this draft is imported. It is an architecture/reuse lead requiring
  provenance, generation and recovery review, not a ready N100 backend.
- [PR 222](https://github.com/RenkuOS/Source/pull/222) / aggregate 229 concern
  upstream brightness changes; not Alder Lake acceleration.

## Migration scope and first patch

Carry the verified N100 GART pair and capability safeguard as separate patches,
retaining RenkuOS's unrelated Intel fixes. Re-run source tests and native linked
hook tests, with an unpatched control that must fail. Compile target components
without installing them. Then use a separate RenkuOS VM for non-hardware tests.

Physical display and GPU execution still require the earlier hardware-test gates.
Firmware, Gen12 logical contexts, protected command submission, buffers/fences,
hang recovery and app_server integration remain missing work. No OpenGL/Vulkan
support claim follows from this migration.

Work ownership: source base `source/gpu-reference`, source worktree
`source/gpu-worktree`, project worktree `gpu-driver`, branch `gpu/renkuos-n100`.
Astra High #1 owns Wi-Fi/shared coordination/VM provisioning. Both OS projects
use the same authoritative Fedora resource locks; a directory rename must never
create a second lock domain for the same hardware.
