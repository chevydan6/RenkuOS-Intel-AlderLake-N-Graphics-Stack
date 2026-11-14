<!-- SPDX-License-Identifier: MIT -->
# Native shmem file ownership — 2026-11-14

Source candidate: [`015f1632c04212a6c6f713c5a458e361edaf72d8`](https://github.com/chevydan6/haiku-intel-gpu/commit/015f1632c04212a6c6f713c5a458e361edaf72d8),
tree `74e4ea5ae0be45dc211dcc79bac747e8056e2528`. The exact candidate now compiles
with native GCC/G++13.3 and runs on the physical Intel N100, RenkuOS
`hrev60072+518`, booted from the preserved RenkuTest USB image.

Four version10 completion/memory runs pass **57,786 checks each**; four VMA
regressions pass **650 checks each**. Total: **233,744 checks, zero functional
failures**. The kernel/client report is128 bytes. Each run creates/frees two
shmem files and runs two file readers for512 iterations, in addition to the
retained completion, bitmap, RCU, lockref, sequence, page, folio and store cases.
Native C++ compilation, kernel export linking, successful diagnostic unloads,
all13 production component hashes and unchanged app_server PID127 are recorded.
See [summary.json](summary.json), [native-run.log](native-run.log),
[native-build.log](native-build.log), and the lifecycle/binary hash records.

## Implemented behavior

Scoped Linux-facing `file`, `inode` and `address_space` views now own real native
anonymous backing. `shmem_file_setup`, `get_file`, `fput`, mapping GFP mask helpers,
`shmem_read_folio_gfp` and page-read wrappers have actual implementations. They
are native adapter objects, not binary copies of Linux VFS/VM layouts.

Final file close prevents new store reads. Existing folios and in-flight pins
retain the native area and mapping metadata until final unpin; the joined
release worker then deletes the area and invokes the metadata callback.
Failed deletion retains ownership and retries. Native tests verify zero-fill,
logical EOF/rounding, data retained across unpin/re-pin, canonical concurrent
folios, balanced file references, close with a live folio, area presence before
retirement and actual absence afterward. VM_NORESERVE maps to the pinned native
overcommit and no-up-front-commit flags; the live area's overcommit property and
B_NO_LOCK wiring were checked. This is not swap-pressure or reclaim evidence.

Only ordinary sleepable GFP_KERNEL reads are supported here. Other allocation
masks and atomic contexts return EOPNOTSUPP, preserving caller constraints.
In particular, i915's HIGHUSER/reclaim/no-retry policy still needs a genuine
native adaptation. No successful dirty/accessed, reclaim, writeback/truncate,
unevictability or user-mapping stub was added.

## Host tests and review

The focused test first failed for missing ownership/shmem implementations. A
separate oversized-reservation test failed before the signed-span guard was
added. Restored positive runs pass under ASan/UBSan, and six mutations are
rejected: early close ignoring pins, ignoring in-flight pins, lost noreserve
policy, duplicate callback, lost file reference, and ignored GFP constraint.
The combined suite passes57,729 checks plus its existing mutation controls.
All100 first/repeated host/target header cases and the18 scope rejections pass;
the four unsupported-operation link controls remain failures as intended.
All118 imported Linux files remain byte-exact.

An independent read-only review found no Critical/Important issues and reran
the focused sanitizer/mutation suite. Its one minor callback-context wording
issue was clarified: final put uses the release worker; explicit unique-owner
destroy is synchronous. The reviewer did not access the physical machine;
the native build/runtime checks above were performed afterward. Full GPU,
arbitrary GFP, mmap and reclaim behavior remain required future work, not claims
of this checkpoint. See [review.md](review.md).

## Reproduce and limitations

Generate a complete bundle with the existing `completion/prepare-bundle.py`
using the pinned Linux and RenkuOS checkouts. This run verified all7,116 bundle
files before build and afterward. Use [visible-run.sh](visible-run.sh), adjusting
its fresh native directory. SSH executed the native commands while fullscreen
RenkuOS Terminal displayed the live log through NanoKVM, as authorized by the
owner. The physical-test lease was released afterward. No production graphics
driver was replaced. SSH remains available on port2222 for this boot.

The native clock still reports October1; the actual UTC session date is
November14. Content and binary hashes identify the run. Raw evidence and
binaries remain under `/boot/home/i915-shmem-native-001` and locally under
`/home/dan/projects/i915-shmem-native-evidence-001`; the private archive digest
is recorded in `summary.json`. Credentials, raw syslogs/process captures and
binaries are excluded from this publication.

All four full original source attempts still fail. GTT/ring require real
process-mm/task services in sched/mm.h; physical/shmem GEM reach the explicit
VM bounds rejection and missing asm/page.h through their own swap.h include.
See [whole-build](whole-build) for complete errors. The full original i915
driver, GPU DMA, command submission/retirement and acceleration remain unfinished.
