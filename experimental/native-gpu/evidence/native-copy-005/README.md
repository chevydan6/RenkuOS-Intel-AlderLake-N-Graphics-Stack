<!-- SPDX-License-Identifier: MIT -->
# Native N100 GPU copy: verified on RenkuOS

2026-11-14. The actual N1008086:46d1 revision0 ran the fixed native BCS workload.
The client exited0 and reported PASS. This is GPU execution, not simulated IO.

| Evidence | Actual result |
| --- | --- |
| Private PPGTT copy |4096bytes;1024/1024dwords match|
| Source/destination FNV |2bb2fdc5 /2bb2fdc5|
| GPU nonce/complement |032d377f /fcd2c880|
| Ordered breadcrumb |1|
| CSB activation |03ff800000008019|
| CSB idle/completion |0000800003ff8018|
| BCS MI_MODE |00000200|
| Engine error / GT fault |0 /0|
| Forcewake group |status0, Complete stage6; all four requests cleared|
| Client ioctl / report |0 /0, final stage14|

The CPU initializes source and poisons destination. Only the GPU BLT copies to
destination; all1024output words are compared after ordered completion/cache
visibility. Pages are distinct in the owned PPGTT tree. Nonce/sequence writes
are separate GPU commands. Existing Intel hardware packets and native helpers
are retained; no Linux scheduler/device compatibility layer runs this workload.

The prototype permits one attempt per boot and retains request backing until
reboot, including success. It does not claim reusable submission/backing
reclamation, EU shader execution, OpenGL/Vulkan or media acceleration. The
forcewake group releases normally after verified inactive context status.

See runtime.log, build.log, summary.json, installed.sha256, binaries.sha256 and
kernel/compiler records. source-before.log/source-after.log verify the actual
bundle; unchanged-after.log verifies the thirteen original display/observer
components, and app-server.log confirms the desktop server remains present.
Earlier failure logs explain PCI snapshot and CSB cursor adaptations. No
exhaustive test program was run. The cache-equivalence log records the focused
comparison of all64MOCS/L3 entries against the pinned Linux reference.

Exact build-input/binary archive:
/home/dan/projects/native-gpu-execution-evidence-005.tar.gz, SHA256
569bc2751afb82ac8eb4c32b25ddf82ce1cebe6deafad636e787353411d78780.
It also remains on the laptop under /boot/home. The published reader's comment
about matching the initial cursor was clarified after bundle creation; its
executable code is identical to that in the exact archive.

Native source/build/client: /boot/home/native-gpu-execution-005. After a fresh
boot with the matching installed owner/bridge, run `build/gpu_copy_sample` once.
The physical lease was released after verification. Native wall time is wrong;
hashes identify the build and this evidence snapshot.

Published process/compiler text has insignificant trailing whitespace normalized;
the raw archive preserves the exact native files.
