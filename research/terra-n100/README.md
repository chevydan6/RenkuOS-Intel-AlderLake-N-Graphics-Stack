<!-- SPDX-License-Identifier: MIT -->
# Terra N100 evidence

Independent research and host-only validation, 2026-11-07. No production writes,
physical deployment or GPU execution by Terra. Required reports are in repository
root. Shared coordination is `/home/dan/renkuos-development/gpu-driver/coordination`.

The source archive preserves copied upstream notices. It is research material,
not driver code imported into RenkuOS. Linux pin and SHA-256 values are in
evidence/linux-references.json; initial uncommitted RenkuOS snapshot hashes are
in evidence/renku-snapshot.json. Later correction snapshots have separate manifests.
Raw sources are archived rather than added as thousands of source-diff lines.

To restore raw evidence in a clean checkout, from this directory:

```
tar -xzf evidence-sources.tar.gz -C evidence
python3 check_lock.py
python3 check_hash_publication.py evidence/gart-r2-corrected.cpp
python3 evidence/astra-threaded-reviewed.py evidence/gart-corrected.cpp
python3 evidence/astra-recovery-reviewed.py evidence/gart-r2-corrected.cpp --guarded-backend
```

`check_lock.py` verifies that the old source reaches the blocking semaphore path;
it treats that detection as the expected result, then verifies a one-acquisition
control. `--source-root PATH --expect-fixed` tests a corrected worktree read-only.
Its semaphore stub reports the path instead of hanging the host.

Threaded negative control uses the old snapshot and must fail its bounded gate.
Hash negative control uses the old snapshot and must fail its ownership assertion.
Do not rerun aborting negative controls with core dumps enabled; captured logs
record their expected failures. All ordinary pass/fail checks are host simulations
of OS/backend services. Sanitizer success does not prove hardware memory safety.

New tests are MIT; copied source retains its original license and attribution.
No production implementation has been ported from these references.
