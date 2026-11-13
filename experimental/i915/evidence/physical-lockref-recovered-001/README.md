<!-- SPDX-License-Identifier: MIT -->
# Native lockref/VMA validation after recovery

The user returned RenkuOS to the desktop. NanoKVM input worked again; no cause
of the earlier USB stall is inferred. Builds/tests ran visibly in fullscreen
Terminal. Reused /boot/home/i915-lockref-001 retained its source identity and
passed all7078 checksum entries before and after tests. Completion rebuilt and
passed four47056-check runs, actual signals and unload0. Historical failed VMA
and earlier completion runs remain in the external raw archive, excluded here.

The revised VMA fixture was exported separately to avoid another full reference
source extraction. /boot/home/i915-vma-retest-001 passed its source checks before
and after native GCC13.3/SDK518 compilation. Four650-check runs pass with two
concurrent readers, no worker errors and actual unload0:48000 stress operations.
Synchronous callbacks run on two selected enabled CPUs and retain the100ms
bound. All eight runs total190824 passing checks. Both graphics lists remain
unchanged. The prior failed rendezvous evidence is retained in physical-lockref-001.

Whole native original compiler attempts now match the earlier cross boundary:
ring/GTT fail in sched/mm.h on real process-mm/task integration; physical GEM
fails at seqcount/seqlock declarations and asm/stat.h. Exact errors and exits1
are retained. This checkpoint validates111 original imports and the lockref/GFP
adapters; subsequent sequence-lock work is a separate source snapshot. It does
not demonstrate a full i915 build or GPU execution.

Raw archives are under /home/dan/i915-vm/lockref-recovery-001; result.json records
hashes. Native sources: original bundle002 in lockref-port-001 and the separate
VMA bundle in lockref-recovery-001. Temporary access remains active for the next
porting test; no final SSH/lease cleanup is claimed at this intermediate point.
