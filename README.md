# RenkuOS Intel GPU development

Astra High #2: private Intel N100 / Alder Lake-N graphics development for RenkuOS.
Haiku research, patches and tests are preserved on `gpu/alderlake-n`.
**No native GPU execution or acceleration has been demonstrated.**

Verified hardware: Intel N100, host bridge `8086:461c`, GPU `8086:46d1`
revision 00, Fedora 44 / i915. The Haiku beta6 x86_64 VM has QEMU VGA / VESA,
not the physical Intel GPU.

The first two patches correct the missing host-bridge/GPU pair in the inherited
Intel GART module and stop advertising unsupported legacy 2D hooks. Both compile
from RenkuOS source in the Haiku beta6 guest; 185 hook checks pass and original
controls reproduce the bugs. A third patch corrects the Gen8+ GGTT BAR offset
and validates its range; 14 host arithmetic tests pass. Its native build is
pending. None of these patches is ready for physical installation: page-table
entry width, stolen-memory ownership and initialization still need work.

See [RenkuOS migration assessment](RENKUOS_MIGRATION.md), [initial technical report](DRIVER_RESEARCH.md), [hardware](HARDWARE.md),
[architecture](ARCHITECTURE.md), [build](BUILD.md), [testing](TESTING.md),
[performance](PERFORMANCE.md), [known issues](KNOWN_ISSUES.md), and
[roadmap](ROADMAP.md).
See also the [Iris interface requirements](MESA_INTERFACE.md) and
[RenkuOS component results](results/2026-11-04-renkuos.md).

Haiku source baseline: `4f1074d5e14f59e0f5e5c4938c8a50d1290fa59c`.
Mesa research baseline: 26.0.7, `27cdb9ff9315a904b9827f9e711e68bb5e856e50`.
RenkuOS source baseline: `4799bb5a188c4bb6bbef967287640cc21a061a0d`.
Source patches retain original licenses. Haiku's upstream AGENTS.md rejects
AI-assisted submissions; this is the user's explicitly requested private
project. RenkuOS explicitly permits machine assistance with disclosure, testing
and contributor accountability. No upstream submission has been made.

Raw logs, SSH configuration, credentials, VM images and generated binaries
are excluded from version control. Only reviewed evidence is committed.
