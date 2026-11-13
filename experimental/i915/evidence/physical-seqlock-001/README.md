<!-- SPDX-License-Identifier: MIT -->
# Original sequence-lock native validation

Actual N100, four online CPUs, GPU8086:46d1 untouched, hrev60072+518. GCC13.3 and
SDK518 built the new diagnostic module/client visibly in fullscreen Terminal
through NanoKVM input. Four47087-check completion runs and four650-check VMA
regressions pass:190948 checks, zero failures, real signals and actual unload.
Eight sequence workers perform16000 writes and16000 accepted stress snapshots.
A separate reader/writer handshake requires one actual stale-sequence rejection
per run, independently of stress scheduling. Counter transitions/wraparound,
exclusive readers, IRQ variants/restoration and mutex-associated preemption
are also tested. VMA retains48000 stress operations and two overlapping readers.

Native directory: /boot/home/i915-seqlock-001. Report version6 adds real sequence
worker/write/read/required-retry metrics. completion/ and vma/ retain exact runs
and lifecycle logs; source-checks.json records7083 matching entries before and
after compilation/tests. Native compiler output retains known upstream find_bit
warnings under the existing narrow exception; no warning-free claim is made.

The runtime archive omitted6783 unchanged reference files already present in
/boot/home/i915-lockref-001/reference. Host export byte-compared every omitted
file against the prior native-tested bundle; a native reference symlink reused
that tree. The FULL source manifest was retained and checked, including every
referenced file. runtime-export.txt and result.json record archive identities.
All113 manifested imports remain unchanged. Raw archives/bundles are under
/home/dan/i915-vm/seqlock-port-001, outside Git; binary hashes are retained here.

Whole native compiler attempts still exit1: physical GEM now fails only at the
next fatal include asm/stat.h, with previous sequence-lock type errors gone.
Ring/GTT retain Linux sched/mm.h process-mm/task errors. These are compiler
boundaries, not full-file successes. No full i915 build or GPU execution is
claimed. See ../../completion/README.md for the supported sequence-lock scope.

final-terminal.jpg shows successful suites/checksums and SSH shutdown0. A later
SSH connection was refused, the lease released, and USB media/UDC remained
unchanged; access-cleanup.json records actual cleanup observations.
