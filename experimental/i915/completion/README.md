<!-- SPDX-License-Identifier: MIT -->
# Original Linux completion and simple wait queues

The complete Linux v6.6 `kernel/sched/completion.c` and `swait.c` algorithms,
including their public headers, are byte-exact imports. A forced include supplies
the surrounding declarations normally supplied by Linux's sched/build_utility.c.
Native event, task-scope and deadline adapters connect them to existing RenkuOS
semaphores, IRQ spinlocks and monotonic time. No GPU algorithm is replaced.

This supplies counted completion, complete_all/reinitialization, blocking,
timeouts, interruptible and killable waits for registered native entry scopes.
A wake before OS blocking is retained by the semaphore; stale wake credits are
drained before publishing the next wait. Original queue code removes a waiter
when waking it, preserving distinct wakeups for consecutive completions.
The port's tick unit is explicitly 1ms (HZ=1000).

The actual N100 test compiles and loads these complete original implementations.
It checks pre-signaling, timeout/no-token behavior, two queued waiters, counted
wakeups, broadcast/reuse, IRQ-disabled complete/complete_all and scope cleanup.
User-thread SIGUSR1 interrupts an interruptible ioctl wait with Linux -ERESTARTSYS;
a killable wait stays blocked through that ordinary signal and finishes on a
completion. Killing a waiter was not tested. No GPU interrupt or GPU work occurs.

Scope limitations remain explicit:

- Task records are caller-owned entry scopes, not general Linux scheduler tasks
  or reference-counted task handles. Waiters must finish before unregistering;
  the callback releases their native event before module unload.
- First blocking entry creates a semaphore in sleepable context; creation failure
  is currently fatal, as are invalid context/lifecycle operations. Production
  entry/error handling still needs integration before this is a full driver.
- Four task states are supported: running, uninterruptible, interruptible and
  killable. Native acquisition checks pending signals; the adapter records an
  interruption for that scope/wait mode. It is not a general signal subsystem.
- Wake CPU-placement flags are advisory and native scheduling chooses placement.
  IO waits use the same real native blocking path without Linux blkio accounting.
- Host tests use real pthread/POSIX synchronization and model IRQ state. Host
  kill-only signal behavior is not claimed; the native backend uses the verified
  B_KILL_CAN_INTERRUPT service. General Linux wait.h methods remain unresolved
  unless supplied by the imported swait/completion code.

The same checkpoint adds original x86-64 UAPI scalar types, generic do_div,
kconfig helpers and lockdep-off types, plus real atomic bit operations/raw IRQ
locking. Instrumented lockdep and PREEMPT_RT builds fail explicitly.

Host sanitizer test and a lost-count mutation control:

```sh
python3 experimental/i915/completion/run-host-tests.py --output NEW_DIRECTORY
```

Native reproducible bundle (the Linux checkout must contain its include and
i915 source trees; every exported regular reference file is checked against the
pinned Git blob before copying):

```sh
python3 experimental/i915/completion/prepare-bundle.py \
  --linux-source /path/to/pinned-linux --output NEW_DIRECTORY
# Transfer/extract payload.tar.gz; in actual RenkuOS Terminal:
sha256sum -c payload.sha256
bash completion/build-native.sh > build-native.log 2>&1
bash completion/run-native-tests.sh
bash completion/attempt-whole-native.sh
```

The final command preserves real compiler failures for the complete original GEM
and ring files. They still fail; successful completion-library tests do not mean
the full i915 driver compiles. See ../evidence/physical-completion-001.
