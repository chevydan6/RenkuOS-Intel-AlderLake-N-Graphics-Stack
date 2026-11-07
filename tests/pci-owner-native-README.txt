SPDX-License-Identifier: MIT
A26: controlled-boot PCI observation through the retained N100 owner

Production source: source/gpu-next-worktree, owned by Astra. Terra reviews in a
separate workspace. Patch0089 is incremental against the exact pre-change owner
snapshots in evidence/physical-pci-owner-001. It preserves v0 ABI and exports v1
in the same image. v1 gets/retains v0; do not load a second owner image or rerun
v0's initializer. The existing state header matches the recorded copy in
evidence/owner-adapter-001/IntelApertureLeaseState.h.

Pure host checks:
  g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I workstreams/gpu-next tests/pci-power-test.cpp -o /tmp/pci-power-test
  /tmp/pci-power-test
  python3 tests/owner-observation-host.py --source SOURCE --output ISOLATED_DIR
The harness extracts actual production observation functions, not a rewritten
model. PCI callbacks and kernel mutexes are mocked. Deterministic barriers test
close during admission/retirement, concurrent End and failed GART ownership.
Those results remain simulation even when the same harness runs on the N100.

Native build uses the existing089 bundle Makefile/SDK. Copy the reviewed owner
source/private headers and fixture files into tests/, then:
  make -f Makefile -f tests/pci-owner-native.mk \
    build/intel_memory_owner_v1_candidate build/intel_pci_owner_bridge \
    build/pci-owner-native-client
Its explicit -Itests precedes baseline includes, selecting the new owner ABI.
Kernel targets use existing kernel flags and link with --no-undefined.

Physical deployment replaces the retained owner only across reboot; a file
replacement cannot update the current B_KEEP_LOADED image. Hold canonical locks
through hardware operations, verify source/binary and unchanged display/GART
hashes, preserve current owner on USB and host, and keep the known boot image.
Target owner: /boot/system/non-packaged/add-ons/kernel/bus_managers/intel_memory_owner
Test bridge: /boot/system/non-packaged/add-ons/kernel/drivers/bin/intel_pci_owner_bridge
Device link: drivers/dev/misc/intel_pci_owner -> ../../bin/intel_pci_owner_bridge
Backup: /boot/home/gpu-memory-089-001/recovery/pci-owner-a26/intel_memory_owner
Physical reset recovery requires Dan: NanoKVM has no exposed ATX/power tool.

After reboot, run build/pci-owner-native-client visibly through NanoKVM. Each
ioctl performs two Begin/duplicate/End/stale sequences and copies results only
after every admitted lease has had End attempted. It accepts no user BDF, lease,
register offset, pointer or command stream. A 10-second alarm bounds userland,
not an uninterruptible kernel PCI read; retain external observation/recovery.
Check six archived physical sessions and paired memory regression in the evidence.

No PCI writes, GT MMIO, forcewake, DMA or GPU commands. D0/config checks are
samples in a controlled boot, not a system-wide power reference. Refer to
research/n100-submission/PCI-POWER-OBSERVATION.txt for limits and execution gates.
