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
