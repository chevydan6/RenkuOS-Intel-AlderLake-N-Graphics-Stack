# A34 reset-safety diagnostic candidate — software evidence

Patch0096 adds a retained snapshot of the existing reset safety checks and a
read-only v8 query. It preserves v0–v7 layouts and the reset sequence. The updated
diagnostic bridge/client protocol is version2 and includes failure details.

* 140 actual-owner host ASan/UBSan cases passed.
* Fresh 96-patch source reconstruction matched development source exactly;
  843 payload hashes passed.
* Locked local VM native build passed 140 owner / 431 reset / 964 group /
  4101 planner / 84 forcewake / 37 inventory cases, exit0.
* Native input manifest and supplemental hashes were checked before build.
* Kernel/SDK hashes match the physical baseline. Artifact hashes, ELF64/x86-64
  type, dependencies and required exports were verified; see artifact-review.json.

Exact bundle: `build/a34-reset-safety-readiness-001/`.
Collected binaries: `build/a34-native-001/build/`.
The owner filename remains `intel_memory_owner_v7_candidate` because the reset
API is v7; it additionally exports the v8 read-only diagnostic query.

Subsequent A34 physical reset and postchecks passed; see
`tests/evidence/physical-bcs-reset-002/`. A33's earlier safety-check failure
remains unexplained; see `tests/evidence/physical-bcs-reset-001/`.
