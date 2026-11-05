# RenkuOS Intel GPU development

Astra High #2: private Intel N100 / Alder Lake-N graphics development for RenkuOS.
Haiku research, patches and tests are preserved on `gpu/alderlake-n`.
**No native GPU execution or acceleration has been demonstrated.**

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

Raw logs, SSH configuration, credentials, VM images and generated binaries
are excluded from version control. Only reviewed evidence is committed.
