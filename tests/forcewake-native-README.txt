SPDX-License-Identifier: MIT
A28: standalone primary forcewake diagnostic, no GPU commands

Patch0091 extends A27's retained owner without changing v0/v1/v2 layout. v3
admits directly into Handshaking; only exact bit0 enable/clear are issued to
0xa188. Readback is fixed0x130044. Unknown ownership retains request/pin/report
until reboot; no close, query or reopen resets it. Fallback15 is never written.

Pure host tests: build tests/forcewake-test.cpp with -Iworkstreams/gpu-next,
C++17 warnings-as-errors and ASan/UBSan. Actual owner mocks use
python3 tests/owner-observation-host.py --source SOURCE --output ISOLATED_DIR.
These are simulated hardware, even when run natively on the physical CPU.

Native bundle: copy reviewed owner source and private headers plus fixture
files into tests/. Build using Makefile and tests/forcewake-native.mk targets
build/intel_memory_owner_v3_candidate, build/intel_forcewake_bridge and
build/forcewake-native-client. Kernel targets link --no-undefined against the
native kernel import library. Copy the reviewed generated test include to run
the actual-function harness natively. Final sources and binaries must match.

Physical: preserve A27 owner on USB and host before same-path replacement and
fresh boot. Existing owner is B_KEEP_LOADED; file replacement alone cannot update
it. Hold canonical resource locks and keep visible NanoKVM observation. Check
candidate/installed hashes, unchanged display/GART artifacts and live PCI first.
One invocation of build/forcewake-native-client makes one transaction; there is
no global one-shot limit after success. Do NOT auto-retry on error. The user alarm
does not interrupt a stuck kernel MMIO instruction. Physical reset needs Dan;
NanoKVM exposes no ATX/reset tool. Preserve known boot and module recovery copies.

Test node /dev/misc/intel_forcewake is published by intel_forcewake_bridge at
drivers/bin; drivers/dev/misc/intel_forcewake -> ../../bin/intel_forcewake_bridge.
No user address/lease/packet input. The result carries version/module/transaction
status plus full phase/error/cleanup/ownership/raw ACK/count/elapsed diagnostics.
Success requires observed0->1->0, two writes, Released/Complete and no errors.
Capture display/process/PCI status afterward. Memory regression is a separate
check; run only after the handshake succeeds. No DMA, rendering or acceleration
is demonstrated by this diagnostic. Design: PRIMARY-FORCEWAKE-TRIAL.txt.
