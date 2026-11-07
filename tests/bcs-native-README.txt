SPDX-License-Identifier: MIT
A29 candidate: fixed BCS inventory while GT forcewake is held

Status: host tested and independently source reviewed. Native compile and
physical sampling have not run. Target connectivity failed before source
transfer/build; the candidate was never installed. Restore laptop access first.

Patch0092 extends the reviewed A28 owner. Preserve v0..v3 interfaces; v3 uses
the null inspection hook and keeps its old cases/budgets. v4 has only fixed raw
sampler/report outputs, with no user offsets, callbacks, commands or addresses.
Report and request pin cover acquisition, five reads, release and final owner
checks. Typed uncertainty keeps Unknown/pin; raw output commits only on success.

Host tests: tests/forcewake-test.cpp (84), bcs-inventory-test.cpp (37), and
owner-observation-host.py against the candidate source (48). These are mock/RAM
tests, not physical GPU evidence. Use C++17 -Wall -Wextra -Werror with ASan/UBSan.
The unchanged v3 cases are part of those totals, not new hardware reliability.

Native existing089 bundle: copy owner/private headers, fixture and generated
actual-function test include into tests/. Build with Makefile plus
tests/bcs-native.mk targets build/intel_memory_owner_v4_candidate,
build/intel_bcs_inventory_bridge and build/bcs-native-client. Kernel link must
retain --no-undefined. Run the same mock tests natively and record final hashes.
Do not install before independent final source/artifact review and backup.

Physical trial: preserve old e195cd... A28 owner on USB and host; checksum gate
native/installed/baseline artifacts, reboot to load the B_KEEP_LOADED candidate.
Hold canonical locks and use visible NanoKVM. Bridge node is
/dev/misc/intel_bcs_inventory via drivers/bin/intel_bcs_inventory_bridge and
drivers/dev/misc/intel_bcs_inventory -> ../../bin/intel_bcs_inventory_bridge.
One client invocation performs one transaction; repeated calls are not globally
disabled after success. Stop on error, no automatic retry. User alarm cannot
interrupt a stuck kernel MMIO instruction. NanoKVM exposes no ATX/reset control;
physical reset may require Dan. Never unbind its root USB/media gadget.

Capture raw tail/head/control/mode/message values and owned handshake report,
then check HDMI, PCI, ACK and paired GART regression only after success. These
sequential values establish no engine presence, idle state or readiness. No
engine/ring/reset writes, GPU commands, DMA or acceleration are implemented.
