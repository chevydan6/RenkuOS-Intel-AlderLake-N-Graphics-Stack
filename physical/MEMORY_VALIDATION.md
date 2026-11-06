<!-- SPDX-License-Identifier: MIT -->
# Controlled physical memory validation proposal

Owner: Astra. Independent reviewer: Terra. This procedure is not recorded as
executed. Do not run memory-smoke until the following gates have evidence.

1. Terra disposition of memory fixes R1–R4 and review of this diagnostic/procedure.
   R5 borrowed backing is explicitly excluded: diagnostic uses only owned
   allocate/free ioctls with B_APERTURE_NON_RESERVED. No external-area bind.
2. Verify live target PCI 8086:46d1 and stepping, running kernel, boot medium and
   device node. Reject emulated VGA or another Intel GPU. Check actual loaded
   module paths and hashes; shared ABI version alone does not identify a build.
3. Verify current management and recovery access, including boot-selection and
   power recovery. Historical NanoKVM availability is insufficient. If the
   recovery path cannot be exercised safely, do not deploy or reboot.
4. Preserve current working USB/image and driver hashes. Stage a separate
   candidate and matching intel_gart, agp_gart, driver and accelerant artifacts.
   Preserve the tested build 057 option. Never install onto internal systems.
5. Hold canonical hardware/host locks across staging, physical boot, collection
   and recovery. Coordinate other active driver work before rebooting.

Capture pre-test display frame and kernel log; verify desktop activity and SSH.
Keep app_server/primary accelerant open: this is allocation/free smoke, not
last-close unload or destructive reset testing. Disable all execution experiment
markers; no GT command-publication path is part of this diagnostic.

Run physical/memory-smoke.cpp compiled with the matching native SDK as:
`memory-smoke --run /dev/graphics/<verified-device>`
Record stdout/stderr and exit status. Thirty serial allocation/free cycles use
4 KiB, 8 KiB and 64 KiB, with at most 64 KiB of requested backing live at once.
The tool uses exact driver-returned descriptors, no address guesses, no CPU writes
through GPU mappings, no MMIO and no submitted commands. It stops on any failure.
The 20-second process alarm is not a kernel watchdog. Plan a separate 60-second
observer deadline with verified recovery; no endless remote retries on loss of
management. Do not induce allocation exhaustion or hardware hangs in this test.

On allocation failure, retain logs: GART may retain hidden partial-bind backing.
On free failure, capture the reported descriptor in private logs and do not
attempt guessed cleanup. Restore/reboot through the verified route if needed.
Neither process exit nor ioctl success proves kernel resource counts or GPU idle.

After success, capture fresh kernel log and display frame, verify desktop/SSH,
and compare memory/sem counts if supported by an independently reviewed collector.
Do not equate 30 successful returns with a leak-free or race-free kernel.
Repeat cold boot only after first run evidence is reviewed. Later tests should
cover controlled close/reopen, concurrency and failure injection separately.

Evidence record: physical identity, boot ID/medium, kernel+SDK, source/patch/binary
hashes, exact invocation, observer times, return status, raw logs, display captures,
recovery action and post-test state. Separate VM help-only compilation from this
physical record. Passing this procedure establishes only controlled owned-buffer
allocation/free on hardware, not borrowed-buffer safety or GPU execution.

## Native diagnostic build

From the prepared bundle root on RenkuOS/Haiku, with the diagnostic copied to
`tests/memory-smoke.cpp`:

```sh
g++ -std=c++17 -Wall -Wextra -Werror \
    -I source/headers/private/graphics \
    -I source/headers/private/graphics/common \
    -I source/headers/private/graphics/intel_extreme \
    tests/memory-smoke.cpp -o memory-smoke
./memory-smoke --help
```

Help-only execution opens no graphics device. Preserve the source hash with the
binary before staging. Build 078's first diagnostic compile failed because the
command omitted the graphics/common include directory; its original log is
retained as native.log.attempt-1 in gpu-hdmi-build-055. That attempt is not a
successful diagnostic build.

Corrected build 078 passed native compilation and help-only execution; artifact
finalizer passed using gpu-hdmi-build-055/validation-attempt-2. The allocation/free
path has not run. This does not satisfy the physical admission gates above.
