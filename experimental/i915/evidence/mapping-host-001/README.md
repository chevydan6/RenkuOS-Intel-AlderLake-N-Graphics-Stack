<!-- SPDX-License-Identifier: MIT -->
# Mapping adapter host checkpoint

Actual mapping function bodies from the candidate owner, with the actual
IntelApertureLeaseState class. GART allocation/free and native area services are
modeled. Build uses UBSan with recovery disabled; local sanitizer library flags
are recorded in the mapping README. The tested source was independently rebuilt
from the pinned baseline and candidate patches by prepare-bundle.py.

Successful checks cover alias permissions and distinct CPU/GART address domains,
bounds/overlap, reentry, closure during allocation, each of four allocation and
free failures, failed-output preservation, alias deletion failure and cleanup
retry. Three deliberate generated regressions fail runtime assertions; see
negative-controls.json. These are host tests, not GPU or physical evidence.

Earlier harness attempts and exact generated binaries remain outside Git under
/home/dan/i915-vm/mapping-001/host-test-001 through host-test-004. The first
attempt lacked the system sanitizer linker target; local extracted real runtime
libraries resolved that. A harness initially tried revoking the same display
lease repeatedly; its first-revocation-only correction is retained in the final
harness. Final host evidence: /home/dan/i915-vm/mapping-002/host-test-001/.

The first real N100 attempt compiled but panicked on a direct kernel write to
the user-permission aperture. The corrected source adds a kernel-only clone.
Native build success for that correction does not establish runtime success.
Physical results are recorded separately as available.
