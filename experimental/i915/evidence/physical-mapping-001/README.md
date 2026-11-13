<!-- SPDX-License-Identifier: MIT -->
# Physical owned-GGTT memory checkpoint

Real Intel N100 (GPU 8086:46d1), RenkuOS hrev60072+518, installed matching SDK and
GCC13.3. Both native compiler runs and actual tests ran visibly through NanoKVM
in fullscreen Terminal. SSH transferred source and collected evidence.

Eight invocations passed 38,927 checks each: 311,416 checks, zero failures.
Each invocation holds four non-overlapping, non-reserved GGTT objects containing
19 pages total; 8 sets therefore exercised 32 object allocations/releases and 152
page equivalents. The native interface uses its own Request lease, separate
kernel-only CPU aperture alias and retained cleanup record. Returned GGTT offsets
were0x3c00000,0x3c01000,0x3c02000,0x3c03000, with sizes 4K/4K/4K/64K. Two CPU
patterns were written/read throughout every buffer. This is CPU access through
real GGTT aperture mappings, not GPU command execution or DMA-readiness evidence.
No device publication, GT forcewake/reset or execution path was added to the probe.

The GART candidate issues the N100 UC GGTT invalidation register operation and
retains native cache writeback. native-gart-lifecycle.log captures actual buffer
binding/unbinding for the final four runs. Hardware readback is evidence for this
CPU aperture path; it does not validate engine/GuC translation invalidation,
IOMMU identity, PAT/MOCS, GPU cache visibility or request retirement.

Preserved failures and recovery:

- First native attempt compiled but panicked on the first CPU store: SMAP rejected
  the original user-permission aperture address. Original source is in the first
  payload archive; screenshot path is in result.json. The correction clones the
  same device cache with kernel-only permissions, preserving its WC/UC type and
  retaining original GART addresses for freeing.
- A later attempt did not start: HID and filesystem I/O stalled before Enter was
  delivered, and no test log was created. The official NanoKVM HID-reset operation
  unexpectedly selected its default /dev/mmcblk0p3 media. The original test-image
  attachment and flags were restored immediately. Dan manually restarted the N100;
  its original external RenkuTest boot path and candidate hashes were rechecked.
  No test results are attributed to that interruption. Do not use HID reset
  without preserving/restoring this custom media attachment.
- first-wrapper.log contains four successful mapping runs but wrapper exit1:
  it checked device removal too early and matched historical unload messages.
  Its failure is retained. The final runner waits at most 10 seconds for both
  absent devfs device and a new UNINIT status 0 event from this invocation.
  final-run.log contains four further successful runs and complete cleanup exit0.

final-postcheck.log verifies current diagnostic files/device absent, actual
INIT/UNINIT status 0, unchanged graphics entries, candidate hashes and unchanged
Intel display-driver/generic GART-manager hashes. Original owner/backend binaries
remain checksummed in /boot/home/i915-mapping-001/recovery/. The final mapping
owner and Intel GART candidates remain installed on external RenkuTest; the
native display driver, accelerant and kernel were not replaced. The rollback
script here restores the pre-mapping owner/backend on disk and requires reboot;
its syntax and input hashes were checked, but rollback was not executed.

Exact source archive, native binaries, complete raw logs, original failed source,
and recovery originals are outside Git, under the paths/hashes in result.json.
The final target tree is /boot/home/i915-mapping-002/. Its final runner was added
after the original source archive, with no change to the built native components.
The repository includes that exact runner. Full authentication/network logs and
private connection material are not included in this evidence directory.

Final session cleanup: the dedicated port2222 SSH transfer listener was stopped
through the visible Terminal and a new connection returned refusal. The physical
lease was released. NanoKVM's final media was checked as the original
/root/renku-gpu-test-004/disk.img with ro=0/cdrom=0; HDMI remained live.
