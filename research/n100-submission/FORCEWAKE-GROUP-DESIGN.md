<!-- SPDX-License-Identifier: MIT -->
# A32: retained multi-domain forcewake transaction

The A31 raw fuse sample `0x000e00fe` selects four domains (GT/render/VDBOX0/VEBOX0)
using the pinned N100 platform/fuse rules in `BCS-RESET-DESIGN.md`. This helper
implements their primary-bit handshake; v6 has now passed a physical trial.

## v6 owner integration

Patch 0094 adds `bus_managers/intel_memory_owner/v6`, preserving the earlier
interfaces. Its only entry point performs a fresh GT/fuse bootstrap and the
fixed selected-domain handshake under one retained observation operation pin.
The group helper receives no work callback. The adapter independently checks
selected request/ACK offsets, access direction, exact primary-bit write words,
BAR extent and address overflow. Its safety callback revalidates the retained
mapping, PCI/D0 identity and live owner around each operation.

The group is single-use for this module lifetime. Concurrent/repeated calls
perform no group I/O; an admission-time recheck handles a caller overtaken
between initial checking and admission. Group and bootstrap reports have
separate durable snapshots, so later v3–v5 calls cannot replace the v6 evidence.
Only success publishes the raw fuse to the caller. Bootstrap/group uncertainty
retains the numeric request and operation pin and blocks subsequent diagnostics.
An invalid mapping caught before MMIO instead retires the unused pin and blocks
further admission. Final PCI/owner validation can invalidate an otherwise
completed group, and is reflected in the published error.

The actual-owner host harness passes **92 cases** with ASan/UBSan, covering
the original interfaces plus group success, bootstrap failure, preexisting and
fallback ACKs, set/clear timeout, lost/all-ones ACK, post-group owner failure,
concurrent calls, display/PCI/mapping/clock loss, allowlists and preserved paired
reports after a legacy call. A missing `<initializer_list>` in the first test
build was corrected. Review also found and fixed shared-bootstrap-report
overwriting before candidate packaging. Final-owner fault injection is tied to
the helper's Complete stage, rather than a brittle PCI-read count.

The v6 diagnostic bridge/client records the bootstrap and every selected domain
ledger. The client independently checks the trusted fuse-derived domain plan,
ACK transitions, release states, write counts, elapsed bounds and zero work
callbacks. A userspace alarm cannot interrupt stuck kernel MMIO. Physical
installation and validation remain separate from the native VM build.

`IntelN100ForcewakeGroup.h` computes its fixed domain plan internally from trusted
owner identity/revision/fuse inputs. No raw request/ACK offsets or domain mask are
accepted from a client. Each domain has a persistent ledger: initial/set/final
ACKs, last ACK, read/write counts and ownership state. Attempted ownership is
recorded before each write, including ambiguous callback failure.

Sequence: preflight every selected ACK for clear primary/fallback bits; acquire
each domain in GT/render/media order; verify previously acquired ACKs remain set;
optionally perform one trusted bounded operation; reverse-release the exact
acquired set; verify all selected ACKs clear. Per-domain poll budget is 50 ms
with 512 iterations; total is 1 s; optional operation has 100 ms plus reserved
release time. Absolute clocks are checked for overflow/reversal and before and
after safety validation and reads. A stuck MMIO or callback is not interruptible
merely because software has a deadline.

Any failure stops further MMIO and retains the ledger and remaining attempted
ownership. No guessed clears, fallback stores, broad request masks or automatic
retries. A second invocation on the same instance performs no I/O and preserves
the first report. The future retained owner must keep its operation pin and
mapping/code on uncertainty and block all dependent work. Only error=0 and
stage=Complete establish a successful release transaction; per-domain Released
entries alone are insufficient if a later validation fails.

The first physical integration will expose **handshake only**, with no operation
callback. Reset is a subsequent endpoint after all-domain behavior is observed.
Future reset failure must preserve power and DMA ownership rather than trigger
group cleanup. Neither this group nor the completed GT-only observations are a
general runtime-PM service or firmware-exclusion mechanism.

## Validation

964 cases passed under host ASan/UBSan and natively in the separate local
`renkuos-tests` VM under its transaction lock. Tests cover all eight candidate
media-domain subsets, the actual fuse sample, exact enable/reverse-disable
trace, ambiguous failures at every store, every read's failure/all-ones/fallback,
each safety/clock failure, set/clear timeouts (including frozen clock), stale
ownership, lost ACK, optional-operation failure/overrun and repeated calls.

The first VM staging script added a trailing newline to each input: tests passed,
but the exact input-hash audit correctly failed. That run remains in
`build/a32-forcewake-group-001/`. Corrected staging in a new directory verified
all three input hashes **before** compilation and passed the same 964 cases;
evidence is `build/a32-forcewake-group-002/`. This was a transfer-fidelity
correction, not a test-expectation or production-code change.

The VM has emulated graphics and no N100 passthrough. These are software results.

## Physical v6 result

One guarded A32 trial passed on the physical N100: fresh fuse `0x000e00fe`,
bootstrap Complete/Released in 473 us, followed by all four domains
Complete/Released in **1165 us**. Every selected ACK was 0→1→0, with exactly
two writes per domain and no work callback. Postchecks passed ACK/PCI and ten
paired-GART rounds; desktop/SSH and preserved kernel/display/GART hashes were
verified. The hardware lease was released. Evidence and exact identities:
`tests/evidence/physical-forcewake-group-001/`.

This completes the handshake prerequisite. The next integration is a separate
reset-only endpoint with the A30 reset ledger, fixed offset/word allowlists and
held-domain ACK validation throughout its trusted callback. Reset has not yet
run on hardware.
