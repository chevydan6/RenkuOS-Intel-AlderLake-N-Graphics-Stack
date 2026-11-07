Actual Intel N100 memory smoke, 2026-11-07

Native089 source bundle was rebuilt on the physical N100. Source manifest,
regression fixtures and artifact finalizer passed. Matching driver, accelerant,
GART manager/backend and retained memory owner were installed on RenkuTest USB.
After reboot, PCI8086:46d1, loaded module paths, accelerant path and file hashes
were verified. Original components are backed up on target and workstation;
original known-working image remains preserved. No kernel replacement.

Thirty serial owned NON_RESERVED allocation/free cycles passed:4KiB/8KiB/64KiB,
maximum64KiB requested live. Same app_server team remained running. NanoKVM
captures show1920x1080 desktop and visible results; Wi-Fi rejoined automatically.
No physical GPU command execution, no new simulated/VM tests at this checkpoint.
This is not concurrency, teardown, leak freedom, borrowed-buffer or reset proof.

Terra independently checked exact rounds/sizes, component hashes/paths, physical
identity, app_server and visual result. Terra did not run the hardware test.
Syslog contains xHCI ConfigureEndpoint max_burst_payload and slot2 stall/recovery
errors; no globally error-free system claim. Raw logs/screenshots and native
build provenance remain under the private path listed in provenance.json.

Next: repeated boot/memory validation with early-HDMI capture, remaining ownership
and interrupt retirement, then reviewed command initialization/submission design.
The full early-boot HDMI sequence has not yet been observed.
