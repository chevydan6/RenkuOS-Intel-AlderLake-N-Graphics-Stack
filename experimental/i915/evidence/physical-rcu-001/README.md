<!-- SPDX-License-Identifier: MIT -->
# Physical N100 RCU checkpoint

Final native compilation and tests ran visibly in fullscreen RenkuOS Terminal
using NanoKVM keyboard input. SSH transported sources and collected evidence.
GCC13.3/SDK518, hrev60072+518, four online N100 CPUs. The final screenshot shows
the suite, checksum success and temporary SSH shutdown. access-cleanup.json
records the released lease and preserved virtual media.

result.json records four runs each: completion46934, VMA648, KPI62, for 190576
checks and zero failures. Completion report version4 adds two RCU workers and
one real heap free after a grace period per run. All modules actually unloaded.
Final directories were completion.Thh3ji, vma.rB814J and kpi.l9HuHs. Earlier
completion.JXngr1 is retained in the external raw archive, excluded from totals.
Four real I915_PORT_LOG native syslog markers are captured in completion/.

Base bundle002 was checked (7068 files). Two logging files were overlaid and
the final module was rebuilt/retested; final runtime manifest verified7064
non-Markdown files. source-checks.json records full manifest/check hashes;
adapter-sources-base.sha256 and adapter-sources-final.sha256 retain the smaller
nonreference inventories. reference-provenance.json records the unchanged
107 original imports. Final sources match bundle003; result.json records
archive hashes. Raw archive is outside Git at
/home/dan/i915-vm/rcu-port-001/native-results-002.tar.gz; native directory was
/boot/home/i915-rcu-002. Evidence includes final build logs and binary hashes.

The RCU implementation uses stronger native RW exclusion, short nonblocking
readers and IRQ-off grace waits. Outer entry requires preemptible task context;
there is no callback queue or complete Linux RCU/progress guarantee. See
../../completion/README.md for the contract and remaining unsupported services.

All three whole original GEM/ring attempts in whole-build/ still exit1 at
Linux generated/bounds.h. No full i915 build or GPU execution is claimed.
Known original find_bit maybe-uninitialized warnings remain under the existing
narrow warning exception; this checkpoint is not a warning-free build claim.
