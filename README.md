# RenkuOS Intel GPU development

> **Active handoff — 2026-11-14:** Continue from [NATIVE_GPU_HANDOFF.md](NATIVE_GPU_HANDOFF.md). The owner abandoned the full Linux i915/LinuxKPI port in favor of native adaptation retaining Intel hardware logic and a real GPU-accelerated RenkuOS sample. New native code and prior work are preserved on the linked continuation/archive branches. See also the [Intel documentation inventory](docs/INTEL_GPU_DOCUMENTATION.md).

> **Repository navigation:** Start with the [documentation index](docs/INDEX.md), [current evidence status](docs/CURRENT_STATUS.md), [native/experimental driver map](drivers/README.md), and [test/evidence map](tests/README.md). The cleanup branch is a staged reorganization; production source and hardware installation are unchanged.

Astra High #2: private Intel N100 / Alder Lake-N graphics development for RenkuOS,
with independent Terra research and validation. Target: `8086:46d1`, revision 00.
Haiku research, patches and tests are preserved on `gpu/alderlake-n`.

**Current status, 2026-11-07:** native 1920×1080 RGB8 HDMI desktop output is
physically validated. A34 completed one bounded physical BCS reset with two reset
rounds and clean release of four forcewake domains; display/memory postchecks
passed. A33's earlier safety-check failure remains unexplained. **GPU command
execution, accelerated rendering and Vulkan have not been demonstrated.**

Patches through0096 implement retained observation/reset diagnostics. The newer
descriptor/status, bounded CSB reader and16KiB context-image helpers passed
2357,135 and715 host/native cases respectively. They are CPU-only prerequisites,
not a wired submission path. Owned execution-memory integration, persistent
DMA/power ownership, cache/TLB policy and engine initialization remain unfinished.

Start with the [current progress and evidence report](results/2026-11-07-reset-context-progress.md),
[task board](coordination/TASK_BOARD.md),
[hardware results](coordination/HARDWARE_TEST_RESULTS.md), and
[command-submission design](research/n100-submission/DESIGN.md).
The [HDMI report](results/2026-11-06-hdmi-integration.md) records the display work;
the later build 057 remains the known-working display baseline.
See [MAINTENANCE.md](MAINTENANCE.md) for module boundaries, resource ownership,
hardware invariants and the cleanup/review workflow.

The [local-model investigation](results/2026-11-07-local-model-investigation.md)
records the failed Luanti/VoxeLibre agent trial, reproducible probes, evidence,
and proposed harness corrections.

The following early milestones are historical snapshots. Their patch counts,
installation state and former hardware-validation restrictions describe those
earlier checkpoints; use the current report above for present status.

Verified hardware: Intel N100, host bridge `8086:461c`, GPU `8086:46d1`
revision 00, Fedora 44 / i915. The Haiku beta6 x86_64 VM has QEMU VGA / VESA,
not the physical Intel GPU.

The first four patches correct discovery, unsupported capability exposure,
GGTT BAR layout and Alder Lake PTE width/bounds. They compile in RenkuOS with
matching kernel headers; 185 native hook checks and 42 GGTT checks pass.
The initialization/cache-mapping and GMS-decoding patches also compile and pass
the native RenkuOS checks. Patch 7 disables the Alder Lake legacy ring lifecycle
and unsupported engine/overlay hooks; 22 native RAM-fixture checks pass, with
an original-code control that reproduces the three lifecycle failures.
Patch 8 withholds the i965 clock-gating fallback on Alder Lake. All eight patches
now pass native compilation and software tests, including the complete Intel
kernel driver and five clock-gating checks with an original-source control.
None is ready for physical installation:
stolen-memory ownership, modern initialization and execution remain unresolved.

Patches 9/10 address neighboring-PTE corruption and premature freeing after
partial bind/unbind failures. Patch 11 releases the pointer table after backing
allocation fails. All eleven patches now compile in native RenkuOS, including
the complete generic GART manager. Its 267 mock-backed checks pass natively and
under host sanitizers; original controls reproduce 48 failures. See the
[GART continuation report](results/2026-11-05-gart-lifecycle.md).

Patch 12 skips unused Alder Lake kernel ring/overlay/status allocations while
preserving cursor allocation. Its [280 checks](results/2026-11-05-kernel-resources.md)
now pass in native RenkuOS and both sanitized/optimized host builds. All twelve
patches compile together; no hardware driver execution has been demonstrated.

The Linux inventory now identifies a 2 MiB protected range at the top of stolen
memory. A [native inventory diagnostic](diagnostics/README.md) also compiles and
passes its QEMU checks in RenkuOS. It reads PCI identity and display metadata;
boot GGTT mappings and physical framebuffer ownership remain unverified. See
the [diagnostic results](results/2026-11-05-boot-inventory.md).
Its separate read-only boot-metadata driver now passes
[82 native kernel-interface checks](results/2026-11-05-boot-snapshot.md) in a
disposable VM, plus 34 descriptor checks. It reports firmware boot data without
mapping GPU registers. This is diagnostic kernel execution, not Intel execution.

The [first physical joint diagnostic](results/2026-11-05-physical-boot.md)
identified the N100 GPU in native RenkuOS and reported the existing 1920×1080
framebuffer. Three metadata snapshots agreed, and the read-only diagnostic
binary hashes matched the VM-tested artifacts. The built-in screen worked;
HDMI/NanoKVM video was unavailable. The Intel/GART patch series remains uninstalled.

An isolated [Gen12 copy-packet experiment](BLIT_RESEARCH.md) passes 60,070
host sanitizer checks. It has no submission path and is excluded from the
driver components; no hardware copy has been executed.

See [RenkuOS migration assessment](RENKUOS_MIGRATION.md), [initial technical report](DRIVER_RESEARCH.md), [hardware](HARDWARE.md),
[architecture](ARCHITECTURE.md), [build](BUILD.md), [testing](TESTING.md),
[performance](PERFORMANCE.md), [known issues](KNOWN_ISSUES.md), and
[roadmap](ROADMAP.md).
See also the [Iris interface requirements](MESA_INTERFACE.md) and
[latest RenkuOS results](results/2026-11-05-renkuos.md),
[GGTT audit](GGTT_AUDIT.md) and [power coordination](POWER_COORDINATION.md).

Haiku source baseline: `4f1074d5e14f59e0f5e5c4938c8a50d1290fa59c`.
Mesa research baseline: 26.0.7, `27cdb9ff9315a904b9827f9e711e68bb5e856e50`.
RenkuOS source baseline: `4799bb5a188c4bb6bbef967287640cc21a061a0d`.
Source patches retain original licenses. Haiku's upstream AGENTS.md rejects
AI-assisted submissions; this is the user's explicitly requested private
project. RenkuOS explicitly permits machine assistance with disclosure, testing
and contributor accountability. No upstream submission has been made.

Unreviewed raw logs, SSH configuration, credentials, VM images and generated binaries
are excluded from version control. Only reviewed evidence is committed.
