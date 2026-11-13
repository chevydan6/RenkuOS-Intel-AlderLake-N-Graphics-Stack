<!-- SPDX-License-Identifier: MIT -->
# Native IRQ/preemption and original helper integration

Actual N100, RenkuOS hrev60072+518, GCC13.3 and installed SDK. Compilation,
kernel tests and full GEM/ring compiler attempts ran visibly in fullscreen
Terminal through NanoKVM. SSH only transferred files and collected evidence.
65 imported Linux files remain byte-exact; GPU algorithms were not rewritten.

Final runs (four per suite, all failures 0, actual unload status 0):

| Suite | Checks/run | Total | Additional coverage |
| --- | ---: | ---: | --- |
| Original completion/swait + helpers/preemption | 149 | 596 | 96 waiter workers, real user SIGUSR1 controls |
| Complete original DRM VMA | 648 | 2592 | 48,000 stress operations, required overlapping readers |
| Older native kernel-service regression | 62 | 248 | 16,000 worker iterations, four native pages/run |

The shared wired per-CPU manager delegates to actual native IRQ masking and
spin/rw locks. It preserves nested IRQ restore requests and overlapping lock/
preemption lifetimes. Tests check real physical IRQ state, CPU identity inside
nested scopes and teardown rejection with active scopes. This is stronger than
Linux task-only preemption exclusion and requires short nonblocking sections.
Linux hardirq/softirq/NMI encodings and interrupt-context classification are not
provided. This does not demonstrate a full i915 build or GPU activity.

Completion signal cases remain -512 for interruptible SIGUSR1 and positive 1949
remaining ticks for killable waits that ignore that ordinary signal. Actual kill
was not tested. Graphics publication is unchanged and the test drivers/links
are removed. Temporary SSH2222 was stopped visibly; subsequent connection was
refused. The physical lease was released. Custom NanoKVM disk.img, ro 0/cdrom 0
and configured USB were verified. Display/owner/GART baseline was preserved.

## Failed attempts retained

The first native compile found the SDK's __packed macro collision. Selecting the
original Linux definition in its wrapper fixes it; see failed-native-packed.log.
No module was installed by that failed build.

The next transfer was interrupted while USB/HDMI/SSH became unavailable. This
occurred before extracting/installing the new module. The user restored the
connection; no cause is inferred. The interrupted archive failed its checksum
and is preserved outside Git. Retransmitted bundle 003 matched its host archive
and every original payload file verified before compilation.

The initial VMA attempt passed 646 checks once, then failed the reader-overlap
assertion (max_readers 1; writer/data errors 0). See failed-vma-overlap. The old
fixture/binary/log archive is identified in result.json. The revised fixture
starts two readers through a bounded 100ms IRQ-off rendezvous before the same
stress workload, requiring actual overlap. Both threads are created beforehand;
no allocation or blocking wait occurs inside that IRQ-off loop. Original reader
and writer/data assertions remain, with two stronger checks. All four 648-check
runs pass. This validates observed overlap; it is not exhaustive scheduling proof.

## Source and binary identity

Base payload 003 is the transferred archive for physical directory 004. After its
initial verification, only vma/native_test.c changed for the rendezvous. Its exact
hash is in vma-test-update.sha256. Final reproducible bundle 004 contains that
fixture and updated documentation. Its 6970 non-Markdown runtime files all matched
the physical source in a final visible SHA256 check. Full payloads, checksums,
raw results and failed archives remain outside Git at paths/hashes in result.json;
small manifests and verification summaries are committed here. Separate binary
manifests identify the saved completion, VMA and KPI build snapshots even though
the build scripts reuse build/. Final screenshot shows the result summaries,
failed full-source attempts, final source verification 0 and SSH shutdown 0.

## Remaining original-source compilation

All three full original translation units still fail (compiler exit 1 each):
GTT lacks asm/ioctl.h; physical GEM reaches device-number collisions, missing RCU
helpers and asm/cache.h; ring reaches bit-scan/bitmap, loff_t and cpumask support.
Unaltered native errors/exits are in whole-build/. No compiler failure was hidden.
Host sanitizer/model, compiler and mutation evidence is in ../preempt-host-001;
component/target-probe regressions are also included here.
