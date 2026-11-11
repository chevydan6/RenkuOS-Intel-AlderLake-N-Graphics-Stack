# Module-manager quarantine entry checks

Run `python3 tests/module-quarantine-host/run.py /absolute/isolated/output`.
The runner reads pinned revision 4799bb5a188c4bb6bbef967287640cc21a061a0d,
composes accepted patches 0100/0101 and new patch 0102, and extracts the actual
init_module, uninit_module, get_module and put_module bodies for ASan/UBSan host
execution. Only kernel infrastructure, image/dependency operations and module
callbacks are stubbed. No public ABI or teardown implementation changes.

ERROR rejects B_ERROR and INIT/UNINIT reject B_BUSY before cached image-reference
adjustment and positive-count acquisition, or public release. The first failed
last put retains legacy B_OK reference-accounting semantics; subsequent public
puts reject without consuming retained custody. Successful READY and permanent
B_KEEP_LOADED reference accounting is unchanged. Tests include positive and zero
counts, preboot image-reference preservation, sentinel output preservation and
recursive public entry from actual INIT/UNINIT callbacks.

Failed INIT resource retention, shared-image sibling custody and split-phase
execution outside sModulesLock remain unresolved gates. This is not complete
ACPI lifecycle retention. Host tests do not prove GPU execution. LeakSanitizer
is disabled (`ASAN_OPTIONS=detect_leaks=0`) for the known traced restriction.
No hardware, VM, deployment, reset, reboot, lock or DMA ownership changes.
