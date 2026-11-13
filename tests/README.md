# Test and evidence navigation

- `tests/*.cpp`, `tests/*.py`, `tests/*-host/`: executable host/VM tests and harnesses.
- `tests/evidence/`: recorded test artifacts, logs, results and manifests. Preserve file bytes and hashes.
- `physical/`: physical diagnostics and trial procedures.
- `installation/`: native build/deployment tooling and validation evidence.
- `diagnostics/`: boot/display diagnostic helpers.

Evidence classification is **HOST**, **NATIVE VM**, or **PHYSICAL N100**. Successful CPU-side tests do not prove GPU execution.

For automated path and test inventories run `python3 tools/map-dependencies.py --output dependency-map.json` from repository root. The mapper is heuristic and has not replaced the native build.
