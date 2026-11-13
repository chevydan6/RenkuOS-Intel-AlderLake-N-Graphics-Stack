<!-- SPDX-License-Identifier: MIT -->
# Original lockref native checkpoint — VMA retest pending

Native builds and execution ran visibly in fullscreen RenkuOS Terminal via
NanoKVM. Physical N100, four online CPUs, GPU8086:46d1 (GPU untouched),
hrev60072+518, GCC13.3/SDK518. completion/ records four47056-check runs with
zero failures, eight lockref workers/32000 iterations, prior RCU/bit/waiter cases,
real signals and actual unload. Report version5 adds lockref metrics.

vma-initial-failed/ is a FAILED regression: third run648 checks, four assertions,
first_line118, max_readers1, worker_errors1. Earlier two runs passed648 each.
The old reader rendezvous missed its100ms deadline; no writer/data corruption
was reported by its stress loop. The runner stopped, so whole-file attempts
and the final source recheck did not execute. Do not combine these as a passing
regression total. Initial source verification passed7078 entries; hash metadata
and nonreference/nonupstream source inventory are preserved here.

The revised VMA fixture uses real synchronous CPU callbacks and is recorded in
the repository plus native-bundle003. It has target-header compile validation,
not native execution evidence. The next bundle was transferred to
/boot/home/i915-lockref-002 and extraction started, but USB keyboard writes timed
out and extraction stalled before a build. HDMI remained at the command line;
SSH could answer some commands. Normal and quick software restarts did not
complete during observation. No cause is inferred, and no HID reset/media
replacement was attempted. A physical RenkuOS restart was requested.

Raw results: /home/dan/i915-vm/lockref-port-001/native-initial-results.tar.gz.
Native tested source: native-bundle002, /boot/home/i915-lockref-001.
Final staged source: native-bundle003 (only VMA fixture differs in runtime code).
Archive hashes and native test totals are in result.json. Source check hash is
in source-checks.json. No full i915 compilation or GPU execution is claimed.
