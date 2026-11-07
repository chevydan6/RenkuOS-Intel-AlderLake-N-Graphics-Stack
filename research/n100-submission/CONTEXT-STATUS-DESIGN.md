<!-- SPDX-License-Identifier: MIT -->
# A35: BCS0 descriptor and one-request context-status ledger

Status: offline helpers implemented in `IntelN100ContextStatus.h`; no MMIO,
publication, allocation or hardware retirement API. This is part of G2/C1,
not a runnable engine. A34's reset success is the preceding physical milestone.
2357 host ASan/UBSan and native VM cases passed. Evidence:
`tests/evidence/context-status-host-vm-001/`.

## Pinned field evidence

Linux `028ef9c96e96197026887c0f092424679298aae8`, files/hashes in
`build/a35-context-research-001/references.json` (archived with test evidence).
Existing LRC/execlists/engine hashes matched the earlier research references.

* `intel_engine_types.h:30–38`: hardware `COPY_ENGINE_CLASS` is **3**;
  the userspace/software engine numbering is not the descriptor class.
  `intel_engine_cs.c:69–75` assigns BCS0 that class, instance0.
* `intel_lrc.h:76–108`, `intel_lrc.c:1501–1534,1573`: legacy64 addressing
  mode3, VALID bit0, FORCE_RESTORE bit2, PRIVILEGE bit8. LRCA bits12–31
  point to context ppHWSP, not the register-image page. Gen11+ tag bits37–47,
  instance48–53, counter55–60, class61–63. Reserved fields stay zero.
* `intel_engine_cs.c:504–507` grants EU-priority support only to render/compute;
  `intel_execlists_submission.c:678–680` adds priority only with that flag.
  BCS descriptor flags here are therefore `0x11d`, without EU priority.
* `intel_engine_cs.c:50,335–339`, `intel_lrc.c:1074–1089`: two-page copy
  base context plus two Gen12 workaround pages. Descriptor encoder validates
  a page-aligned **16KiB** interval below4GiB; it does not allocate that memory.
* `intel_execlists_submission.c:503–514,3579–3583`: software assigns nonzero
  tags and combines class/instance with them. `:160–165` reserves tag0x7ff
  as idle. Fixed helper permits1..2046; counter is always zero.
* `intel_execlists_submission.c:1663–1740`: pre-XeHP Gen12 CSB fields and
  promotion/completion classification. Hardware class is bits3–5, instance6–11,
  incoming tag15–25/counter26–31, detail32–35, wait detail37–43,
  outgoing tag47–57/counter58–63. New-queue bit0 and semaphore mode bit2.
* `:1706–1724`: preemption is not reliably identified by detail5 alone;
  incoming/outgoing context validity and new-queue also matter.
* `:1748–1804,1840–1872`: CSB write pointer can become visible before entry
  data. All-ones poisoning detects stale entries; ordered reads and bounded
  waiting/fallback are separate hardware-service responsibilities.

## Implemented contract

Descriptor construction gates exact N100 identity/revision and arithmetic,
leaves output unchanged on failure, and encodes BCS0/legacy64/privileged initial
force-restore with a non-idle tag. It proves neither mapping ownership nor a
valid context image; callers cannot use it as a publication admission check.

The event ledger is single-use and externally serialized. `Arm` records a
nonzero software generation and non-idle tag. It accepts only:

1. Idle outgoing tag -> expected incoming tag, BCS0, incoming counter0.
2. Expected outgoing tag -> idle incoming tag, BCS0, outgoing counter0,
   detail0 and no new queue.

An all-ones entry returns AwaitingEntryWrite without advancing the event count;
the caller must retry that same CSB slot under a deadline. Foreign tags/engines,
counter mismatch, generation mismatch, unexpected transitions, replay and
additional events after switch-away permanently mark this ledger Uncertain.
The first failure and two accepted raw entries are retained. No rearm/reset
operation is provided. Unused/reserved raw bits are retained; idle-side counters
and switch/wait fields when inapplicable are not treated as identity evidence.

The two-event restriction is deliberate for one isolated direct-ring request;
no preemption, lite restore or multi-port scheduling is supported. This is
stricter than Linux's scheduler, not a replacement for it. Actual status-engine
fields and event ordering on the target still need physical validation.

**SwitchedAway is not retirement permission.** It is one item of evidence.
Ordered completion, nonce/complement, guards, error/idle state, power/mapping
ownership, cache visibility, and a complete DMA lifetime contract remain required.
Generation is CPU metadata, not a hardware field: relabeling a same-tag old event
with a new generation cannot be detected here. The eventual tag allocator must
prevent unsafe reuse and the CSB service must establish freshness/order/no overrun.

## Next integration work

Provide the bounded CSB cursor/reader, HWSP layout and poisoning, cache/barrier
contract, engine error collection, and a durable tag/reset-generation owner.
A36 now supplies the bounded callback reader and CPU HWSP initializer, with135
host/native cases; see `CSB-READER-DESIGN.md`. Production cache/error/ownership
callbacks and the hardware reset/publication protocol remain to integrate.
Build the full16KiB context image (including Gen12 workaround pages), private
PPGTT and separate ring/result/status mappings before engine resume/ELSQ load.
Pure encoding tests establish software behavior; hardware readiness requires
the remaining implementation and recovery evidence.
