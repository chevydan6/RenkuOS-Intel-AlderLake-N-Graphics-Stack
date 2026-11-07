<!-- SPDX-License-Identifier: MIT -->
# A30: bounded BCS reset-only sequence

Status: A34 completed one physical BCS reset and passed postchecks. A33's earlier
safety-check failure did not recur and remains unexplained. Engine initialization
and GPU command execution remain pending.

## Source-derived sequence

Reference: Linux `028ef9c96e96197026887c0f092424679298aae8`; exact retrieved
files and hashes are in `build/a30-reset-research-001/references.json`. Existing
archived hashes for reset, engine, execlists and workarounds matched on retrieval.

* `intel_engine_cs.c:1623-1649`: masked STOP_RING write, prefetch disable for
  Wa_22011802037, MODE_IDLE polling, posting read.
* `intel_reset.c:1708-1719`: workaround applies to Gen11/Gen12 before 12.70,
  including the N100's Gen12 platform. It is not optional just because A29
  observed MODE_IDLE.
* `intel_engine_cs.c:1725-1764`: pending MI forcewake is
  `(MSG_IDLE & (MSG_IDLE >> 16) & 0x3e00) >> 9`. A30 rejects nonzero pending
  requests rather than implementing the additional power-domain-status wait.
* `intel_reset.c:560-606`: request reset and wait READY_TO_RESET; catastrophic
  errors use a different protocol. A30 rejects catastrophic/preexisting request
  or ready state; it does not impersonate recovery of an inherited context.
* `intel_engine_cs.c:396-432`, `intel_gt_regs.h:663-700`: Gen11+ BCS0 reset
  domain is **bit 2**, not the older Gen6 BLT bit 3 (which is Gen11 GuC).
* `intel_reset.c:266-315`: pre-12.70 hardware-domain reset has two successful
  GDRST request/ack rounds followed by 50 us settling. The second round is part
  of this source sequence, not a retry after failure. Each wait is 2000 us.
* `intel_reset.c:764-790,828-839`: Linux holds FORCEWAKE_ALL throughout reset,
  including RC6 exclusion. A29's GT-only handshake is **not** automatically a
  substitute. Resolve domain coverage and controlled-boot power ownership in
  the kernel adapter before connecting the helper to MMIO.

Fixed words/offsets (no user-selectable register or mask):

| Operation | Offset | Word / predicate |
| --- | --- | --- |
| Ring control preflight | 0x2203c | VALID bit 0 clear |
| Stop BCS | 0x2209c | 0x01000100; wait bit 9 set |
| Disable prefetch | 0x2229c | 0x04000400 |
| Pending MI wake | 0x800c | expression above must be zero |
| Reset preparation | 0x220d0 | 0x00010001; wait bit 1 set |
| Domain reset (twice) | 0x941c | 0x4; wait bit 2 clear |
| Cancel own reset request | 0x220d0 | 0x00010000; verify bits 0 and 2 clear |

## Restricted first implementation

Require idle, disabled ring, no preexisting STOP_RING/prefetch disable/reset
request/ready/catastrophic bits, and no other GDRST domain active. The trusted
adapter must establish exact identity, retained exclusive ownership, required
power/forcewake and UC mapping before calling. `safe()` must revalidate those
conditions throughout, without changing registers. It cannot just return true
in production. No assertion that firmware is excluded follows from a callback.

Each register callback is surrounded by deadline/clock and safety checks.
All-ones reads, failed/uncertain stores, clock reversal, power loss or an expired
wait halt the helper. Persist stage, last read, attempted stores and successful
reset-round count before returning. Never issue guessed cleanup after failure.
One helper instance is single-use, even on success; a second call preserves the
original ledger and performs no I/O. Failure must wedge the future retained
service; client close cannot discard that state or its operation pin.

Success means only **ResetObserved**. Keep submission disabled; do not program
ring/context addresses, enable execlists, restart the engine, release DMA backing
or claim context retirement. Context-status service, initialization, VM and
publication are subsequent work. The first reset-only trial has no GPU-owned
allocations, commands or arbitrary hardware recovery paths.

## Validation

Host fault tests exercise the exact helper, with an independent expected write
trace. Inject failure/all-ones at every read, ambiguous failure at every store,
ownership loss, clock regression/overflow/frozen time, timeout in each phase,
preexisting ownership, and repeated calls. Assert failure stops all subsequent
I/O and preserves the ledger. Sanitizers validate host memory behavior only.
Native compilation/mocks precede adapter integration and reset-only hardware.

## Observed implementation validation

`workstreams/gpu-next/IntelN100BcsReset.h` and `tests/bcs-reset-test.cpp` pass
431 cases under host ASan/UBSan (`-O2 -Wall -Wextra -Werror`) and the same 431
cases natively with GCC 13.3.0 on RenkuOS hrev60072+518. Native compilation and
execution were visible through NanoKVM; native exit 0 and input hashes matched.
Logs: `build/a30-reset-research-001/{host-test.log,native.log,native.exit}`.
The compile/mock hardware lease was released after collection. No reset adapter,
module installation or hardware reset was part of these checks.

## Power-domain adapter findings

The freshly retrieved `intel_uncore.c:1347-1433` confirms all six fixed reset
helper offsets lie in GT ranges, but this is register-access coverage only.
`intel_uncore.c:2168-2204` initializes GT, render, VDBOX and VEBOX domains from
engine presence; absent engine domains are pruned later. Therefore substituting
GT-only for `FORCEWAKE_ALL` based on BCS register addresses would drop the
reset/RC6 power constraint in `intel_reset.c:828-839`.

`intel_rc6.c:557-571` also takes FORCEWAKE_ALL when disabling RC6 and takes
control back from GuC first. Writing RC_CONTROL=0 is not a shortcut around the
missing power contract. Next adapter work must identify the actual N100 engine
fuse/domain set and retain a per-domain request ledger across the whole reset,
with ownership-checked reverse release and retained state on uncertainty.
The current A29 inspection callback's 50-ms budget also cannot simply wrap a
multi-domain acquisition. Implement a separate bounded transaction rather than
expanding the read-only inventory callback into a hidden reset entry point.

The domain set is now narrowed from the pinned source: `i915_pci.c:681-688,862`
maps ADL-N to ADL-P with RCS0/BCS0/VCS0/VCS2/VECS0. The media-disable fuse is
`0x9140`, VDBOX bits 7:0 and VEBOX bits 19:16 (`intel_gt_regs.h:632-634`).
`intel_engine_cs.c:751-813` inverts these disable bits for pre-12.55 hardware,
then intersects the platform mask. The XeHP shared-even-VDBOX exception in
`intel_uncore.c:2578-2592` does not apply to this Gen12.0 platform.

`IntelN100ResetDomains.h` implements that offline plan: GT and render, plus
unfused VDBOX0, VDBOX2 and VEBOX0. Requests/ACKs are respectively
`a188/130044`, `a278/d84`, `a540/d50`, `a548/d58`, `a560/d70`. It rejects other
PCI identities/revisions and an all-ones fuse without changing output.

A31 subsequently observed fuse `0x000e00fe` under GT forcewake, with clean
release and postchecks. This selects GT/render/VDBOX0/VEBOX0. A32 implements the
separate v6 single-use group handshake and passes 92 actual-owner host/native
cases plus 964 helper host/native cases. Its physical trial is tracked in
`FORCEWAKE-GROUP-DESIGN.md`; reset remains a subsequent separate endpoint.

## A33 reset-only adapter development

Working source: `build/a33-reset-development-001/source`, copied from the exact
A32 source and packaged as patch0095. The fresh `a33-reset-readiness-001` bundle
matches that source byte-for-byte and passes 813 payload hashes. v7 exposes
one fixed reset entry point and shares the single-use group with v6: whichever
endpoint wins admission consumes it. v6 still selects no operation callback.
v7 selects only the trusted reset helper and cannot accept offsets, masks, ring
addresses, command data or callbacks from userspace.

The reset safety adapter checks PCI/D0/live owner and the retained UC mapping,
the group Work stage and Held ledger entries, and all selected ACKs for primary
set/fallback clear. It repeats PCI/owner validation after the ACK scan and checks
the outer callback deadline and monotonic clock around that work. The reset
helper independently enforces its 10-ms total and per-step budgets. Its read
allowlist is exactly the six helper offsets; its write allowlist is exactly the
five offset/word pairs in the table above (GDRST 0x4 is issued twice).

Failure stops reset writes and prevents group release, preserving the group,
reset report and observation pin. A successful reset report can coexist with
a later group/final-owner failure; callers must require the overall transaction
and group result to succeed as well. No engine restart or DMA publication is
introduced by this endpoint.

113 actual-owner ASan/UBSan cases passed, including the 92 v6/legacy cases plus
reset success with an independent six-store trace, disabled-ring and all-ones
preflight rejection, ready/domain timeout, ACK loss, final owner failure, group
admission failure, replay preservation, v6/v7 mutual exclusion, null arguments,
allowlist/bounds rejection, and concurrent display/PCI/mapping/clock loss during
the first reset store. The same 113 cases passed natively in the locked local
VM; the v7 kernel owner also compiled and linked with warnings-as-errors.
Exact input hashes were checked before compilation. Evidence:
`build/a33-native-owner-001/` and `build/a33-owner-host-002/`. No hardware reset
behavior is established by these mocked transactions or the native build.

The packaged v7 bridge/client/native build recipe is now implemented. The client
requires success of bootstrap, group and reset reports, exactly six reset writes
and two observed GDRST rounds, final own-request cancellation and released group
ACKs. It logs all three ledgers even when the overall transaction fails.
The full native VM build passed 113 owner, 431 reset, 964 group, 4101 planner,
84 forcewake and 37 inventory cases. Kernel/SDK hashes matched the physical
target; binary ELF/dependencies/exports and hashes passed verification.
Physical trial artifacts and A32 rollback are under `build/a33-trial-001/`;
results must be read from its captured logs, not inferred from the VM tests.

## A33 physical failure and A34 diagnostic correction

One guarded A33 invocation reached reset preparation: three stores (STOP_RING,
prefetch disable and RING_RESET_CTL request), 11 reads, no GDRST writes and
zero reset rounds. Reset returned Unsafe at elapsed780us with last read
0x941c=0, attempted store0x220d0=0x10001. The group stopped WorkFailed and
retained all four Held domain requests and the owner pin. No cleanup was guessed.
Desktop/SSH remained responsive. Exact A32 rollback and reboot restored the
baseline; ACK/PCI/BCS inventory and ten paired-GART rounds passed. Hardware lock
was released. Full evidence: `tests/evidence/physical-bcs-reset-001/`.

v7's generic Unsafe code does not distinguish clock, PCI, mapping/owner, group
ledger or individual ACK failure. No cause is established from that code alone.
A34 (patch0096, working source `build/a34-reset-safety-development-001/source`)
records the existing checks in a retained `IntelN100ResetSafety` snapshot:
reason, check phase/domain/count, raw ACKs, clock bounds, PCI capture result,
and expected/observed PCI snapshots. The new v8 module only reads this report,
returns Busy while the transaction is active, and performs no hardware access.
v0–v7 layouts remain unchanged. The diagnostic bridge protocol is version2 and
includes both the reset result and this snapshot.

No reset safety predicate, timeout or write sequence is relaxed. PCI/owner
validation retains the same conditions as the earlier GroupSafe callback.
140 actual-owner sanitizer cases pass, including retained report replay,
read-only/no-I/O report access, active-transaction rejection, and distinguished
primary-ACK loss, fallback ACK, all-ones ACK, PCI, mapping and owner faults.
The subsequent physical result is recorded below.

A34 fresh96-patch reconstruction matched source and verified843 payload hashes.
Native VM build passed140/431/964/4101/84/37 cases with exact input hashes and
exit0. Artifact hash/ELF/export/dependency checks passed. Evidence:
`tests/evidence/reset-safety-host-vm-001/`. The next physical trial should use
this instrumented candidate to distinguish the failed predicate, preserving
A32 rollback and the existing bounded failure/recovery behavior. That trial is
now complete:

A34 physical bootstrap479us; four-domain group2383us; reset1150us,15 reads,
six writes,two reset rounds,ResetObserved. All four domains Released.49 safety
checks/196 ACK reads passed; stable PCI/D0. Postchecks passed clear ACK, PCI
leases, BCS inventory and ten paired-GART rounds. Preserved hashes verified;
desktop/SSH responsive and physical lease released. Evidence:
`tests/evidence/physical-bcs-reset-002/`. The A33 failure did not recur, so its
cause remains unknown. This closes the reset-only prerequisite, not all of G2.
