<!-- SPDX-License-Identifier: MIT -->
# One native request-preparation call — 2026-11-14

Native GCC/G++13.3 on the physical N100, RenkuOS hrev60072+518, compiled and
export-linked the preparation module, bridge and client. One client invocation
passed: mapping/tables/image/cleanup status0, nine owned PPGTT/scratch pages,
96-byte normal tail and104-byte padded tail. Result/completion remained poisoned;
gpu_submitted=0. **No GPU command or accelerated sample was run.**

See [client.log](client.log), [build.log](build.log), [summary.json](summary.json),
binary/source hashes and bridge lifecycle. All26 source files match before/after;
13 production hashes and the resident owner's file hash remain unchanged, and
app_server remains PID127. The bridge unloaded and test files were removed.
The preparation module uses B_KEEP_LOADED: its code/semaphore/dependency remain
resident until reboot, while all prepared request backing was released.

Two actual build issues were corrected: kernel headers precede graphics headers
to avoid the wrong lock.h, and kernel_cpp.cpp needs freestanding/no-builtin flags.
A focused read-only review found no Critical/Important issue in ownership,
serialization, cleanup, ABI or the completion sequence, and confirmed no hardware
publication path. The reviewer ran no tests and accessed no device.

Reproduce from a fresh prepared bundle with native/build-native.sh, then
native/run-native-prepare.sh. Bundle layout: native/, helpers/,
include/native_page_bridge.h, src/native_page_bridge.cpp and native-config/.
INTEL_OWNER_SOURCE defaults to /boot/home/i915-mapping-002/source for the actual
owner headers and kernel C++ support. The runner expects final-payload.sha256 and
trusted prior baseline hashes. Hold the physical lease and show native logs via
NanoKVM. It invokes one client; no exhaustive suite was added.

Raw evidence/binaries: /boot/home/native-gpu-prepare-001 and locally
/home/dan/projects/native-gpu-prepare-evidence-001. Archive SHA256:
27f823141531a46747030d2a53a0d7c59c96a2ff0c0a258d0a5a3d23bc4df91f.
Native wall time is incorrectly October1; session UTC is November14. The physical
lease was released and appliance prompt confirmed after cleanup.

Next integrate engine initialization, DMA/cache admission, ELSQ publication and
completion/retirement under a retained execution owner. Existing mapping/v1 and
this preparation API remain memory-only; their ordinary unpublished cleanup is
not a device-owned request retirement operation.
