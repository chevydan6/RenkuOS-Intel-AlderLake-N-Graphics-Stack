# RenkuOS Intel N100 GPU — documentation index

**Start here:** [Current status](CURRENT_STATUS.md) → [Architecture](ARCHITECTURE.md) → [Reorganization plan](REORGANIZATION_PLAN.md).

## Engineering
- [Build](BUILD.md), [GGTT audit](GGTT_AUDIT.md), [Known issues](KNOWN_ISSUES.md)
- [Mesa interface](MESA_INTERFACE.md), [Power coordination](POWER_COORDINATION.md)
- [Roadmap](ROADMAP.md), [Performance methodology](PERFORMANCE.md)
- [Blit research](BLIT_RESEARCH.md)

## Audit and evidence
- [Repository inventory](REPOSITORY_AUDIT.md)
- [118-patch inventory](PATCH_INVENTORY.md)
- [Patch diff-target batches](patch-audit/)
- [Patch triage and limitations](PATCH_DEPENDENCY_TRIAGE.md)
- [Native implementation map](../drivers/README.md)
- [Test/evidence map](../tests/README.md)
- [Research map](../research/README.md)

## Operational rules
Do not equate host tests, native VM tests or RondoFlow cycles with GPU execution. Preserve known-working physical HDMI and all original evidence. This PR is a staged cleanup, not a completed source migration.
