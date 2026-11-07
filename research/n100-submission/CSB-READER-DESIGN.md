<!-- SPDX-License-Identifier: MIT -->
# A36: bounded CSB reader and engine-HWSP initialization

Implemented: `workstreams/gpu-next/IntelN100CsbReader.h`, tested by
`tests/n100-csb-reader-test.cpp`. CPU/mocked callbacks only; no owner adapter or
hardware publication path is introduced.

## Source and fixed layout

Linux pin `028ef9c96e96197026887c0f092424679298aae8`:

* `intel_engine.h:201–205`: engine HWSP CSB begins at dword0x10 (byte0x40);
  Gen11+ write pointer is dword0x2f (byte0xbc).
* `intel_lrc_reg.h:69–72`:12 entries, pointer mask0xf.
* `intel_execlists_submission.c:2804–2843`: initial head/write pointer11;
  hardware's first post-reset entry is0; poison all12 entries with all ones.
  Linux also resets hardware CSB pointers twice, with posting reads. The CPU
  initializer here does not perform or replace those hardware operations.
* `:1748–1804,1840–1872`: pointer may become visible before entry, poison
  detects stale writes, and a read barrier separates pointer and entry reads.

New reference hashes: `build/a36-csb-research-001/references.json`.
The matching pinned execlists source is in `build/a35-context-research-001/`.
The engine HWSP is a separate allocation from context ppHWSP and result memory.

## Initialization

Accept exactly4096 writable bytes, at least8-byte CPU alignment, nonnull and
nonwrapping pointer arithmetic. Zero the page, poison bytes0x40..0x9f and set
the little-endian32-bit write pointer at0xbc to11. All input checks precede
writes. The caller must supply exclusively owned unpublished backing; size and
alignment checks do not establish ownership or GPU visibility.

## Reader contract

The caller supplies bounded callbacks for time, live owner/generation/error
checks,32/64-bit coherent HWSP reads, acquire visibility and yielding. Raw
MMIO pointers are not accepted. Fixed memory reads are only write-pointer0xbc
and entries0/1 at0x40/0x48. Callback failure retains the outer Failed report.

One `Run` attempt per record, nonzero budget<=250000us and iteration limit
<=65536. Both clocks and the iteration cap apply; a frozen clock cannot hang
the loop. Backward/invalid/overflowing clock, deadline expiry, lost ownership,
read/barrier failure or unexpected events stop the attempt. Clock checks bracket
the safety callback. Raw callback attempts/counts and inner A35 event report
are retained; repeated Run calls perform no callbacks and cannot erase evidence.

Starting from a separately established fresh hardware cursor11, the only
permitted observed pointer progression is11->0->1 (intermediate values may be
skipped). All other values, including high bits, fail. A regression fails even
if the prior entry was still poisoned. Each tail observation precedes an acquire
barrier before its entries. A poisoned entry does not advance the accepted count
or slot; it is retried after yielding within both bounds. Completed slots are
never reread. This first two-event reader does not recycle slots/repoison them.

After both matching A35 events, recheck the tail and safety before recording
EventsObserved. A late third event detected by this read rejects the result.
The backend must independently collect hardware errors in `safe` and establish
cache-coherent, non-torn reads; a callback returning true does not prove those
hardware properties.

## Explicit limits

EventsObserved is CSB evidence only, not nonce/completion/retirement authority.
The outer reader can fail after the inner ledger has observed SwitchedAway;
callers must honor the outer failure and preserve backing.

A modulo write pointer cannot reveal an entire unseen FIFO revolution. The
isolated one-request/no-preemption ownership protocol must exclude that case;
this reader does not implement a general scheduler or prove no overrun. Initial
hardware reset, fresh memory/tags, publication ordering, deadline recovery,
engine-idle/error evidence and DMA ownership remain integration requirements.

## Verification

135 host ASan/UBSan and native VM cases passed: every callback position on the successful path
injected with failure, clock rollback/sentinel/deadline, missing callbacks,
budget/iteration bounds, delayed pointer/entry publication, same-slot retry,
freeze-clock poll cap, foreign tags/preemption/replay, final tail changes,
regression before/after acceptance, exact page contents and unchanged guards.
Exact input, reference and collected native binary hashes were checked.
Evidence: `tests/evidence/csb-reader-host-vm-001/`. These results are software
evidence, separate from physical GPU observations.
