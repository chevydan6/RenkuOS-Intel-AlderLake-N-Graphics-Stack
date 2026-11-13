# RenkuOS Intel N100 GPU — documentation index

**Start here:** [AI/human development handoff](DEVELOPMENT_HANDOFF.md) → [Architecture](ARCHITECTURE.md) → [Build](BUILD.md).

The handoff is a dated, commit-pinned cross-branch inventory of existing code, physical and software evidence, known failures, reproduction commands, and the next integration step. It includes newer experimental mapping work not covered by older native-only status notes. Verify current branch heads before continuing development.

## Engineering
- [Current status snapshot](CURRENT_STATUS.md), [earlier provisional implementation decision](IMPLEMENTATION_DECISION.md)
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
- [Historical staged-reorganization proposal](REORGANIZATION_PLAN.md)

## Scope
PR #2 completed documentation/navigation cleanup and patch/host validation; it did not relocate production source or executable tests. Do not equate host tests, native VM tests, CPU checks on physical hardware, or RondoFlow cycles with GPU command execution. Preserve source and recorded evidence, update the handoff when behavior changes, and keep development focused on a concrete implementation milestone.
