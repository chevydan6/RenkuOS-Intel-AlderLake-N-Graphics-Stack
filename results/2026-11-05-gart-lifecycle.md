<!-- SPDX-License-Identifier: MIT -->
# GART lifecycle continuation — 2026-11-05 JST

The user requested continued autonomous development. Local work continued while
the power agent held OUTAGE_RESERVED for its physical baseline trial. No GPU
host, guest or NanoKVM jobs were started during that reservation.

## Unbind range

The generic manager adjusts both base and length to exclude the already-backed
prefix, but the unbind loop used the original length. For an allocation beginning
at page 2 with length 4 and a reserved prefix of 3 pages, the original code clears
pages 3–6 instead of pages 3–5, modifying a neighboring mapping.

Patch 9 changes the loop bound to the adjusted length. Verbatim production
methods are extracted into a C++ fixture with mock metadata/callbacks. On Linux,
ASan/UBSan passes all 46 checks after the fix. The original source reproduces six
expected failures across three crossing allocations. Fully reserved, ordinary,
end-of-aperture and first-page-failure cases remain covered.

Bundle `renku-016` contains nine patches, all 173 payload hashes verified, and
both positive/original controls pass their expected gates. The full generic
GART manager is added to the native build target; native validation is pending
the power agent's explicit restoration handoff. No physical driver was loaded.

## Next ownership problem

Partial `BindMemory()` success is not recorded until the entire operation
finishes. Cleanup can therefore free owned backing with PTEs still installed.
Additionally, `DeleteMemory()` ignores unbind failures. The next change must
track progress, retain owned pages and aperture metadata when cleanup fails,
propagate errors, and avoid indefinite teardown loops. External buffer lifetime
and pinning require separate treatment; retaining metadata alone cannot pin a
caller's backing pages. Fault-injected tests will distinguish these cases.

## Owned-backing lifecycle

Patch 10 tracks the installed prefix after every successful page bind. Unbinding
proceeds from its tail, updating the prefix only after each successful removal.
Any completed partial clear is flushed even when a later callback fails.
Deletion retains owned backing and metadata on failure; aperture unmap and
module teardown propagate cleanup failure, allowing a bounded retry without
destroying the aperture or looping indefinitely. Rebind and invalid size/state
requests leave existing mappings intact. New borrowed-buffer metadata is cleaned
up on failed binding when the backend permits rollback.

The fixture now executes verbatim production methods and public bind/unmap flow.
All **154** positive host checks pass under ASan/UBSan. The original code produces
**39** expected failures across **103** control checks; additional new-teardown
checks apply only to the patched code. Metadata allocation, page ownership/free,
registry primitives, destruction notifications and locks are simulated. This
does not validate real kernel locking, page allocation or GPU quiescence.
The caller keeps borrowed-buffer backing alive in these tests; kernel pinning
and rollback-failure lifetime contracts still need design before external GPU
buffers can be enabled. Allocation failure before binding and concurrent
aperture-reference lifetime also remain separate audit work.

Bundle `renku-017` contains ten patches and **176** verified payload hashes.
Both host suites and original-code gates pass. Native generic-manager compilation
is prepared but not run while the power agent's OUTAGE_RESERVED remains active.
The previously verified native eight-patch results remain distinct.
