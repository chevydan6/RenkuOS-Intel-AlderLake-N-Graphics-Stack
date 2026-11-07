SPDX-License-Identifier: MIT
A27: physical N100 raw GT ACK observation

Patch0090 extends the retained owner from A26; before snapshots are preserved in
evidence/physical-gt-ack-001. v0/v1 ABI is preserved; v2 gets and retains v0 once.
The same owner BAR mapping now requests explicit UC at fresh boot. No new alias
or live memory-type mutation. Fixed register130044 only; two32-bit reads, no stores.

Host: run owner-observation-host.py against the production candidate source,
using an isolated output directory.26 actual-function tests use mocked PCI and
RAM-backed volatile reads, including the original10 PCI-session cases. They do
not validate hardware read ordering, MMIO access or GPU execution.

Native: copy reviewed owner source/private headers, including IntelN100GtAck.h,
and gt-ack-native fixture files into tests/ of the existing089 native bundle:
  make -f Makefile -f tests/gt-ack-native.mk \
    build/intel_memory_owner_v2_candidate build/intel_gt_ack_bridge \
    build/gt-ack-native-client
Kernel flags and --no-undefined come from the established bundle. Build host
harness natively with -std=c++17 -Wall -Wextra -Werror -pthread and -Itests.

Physical trial: verify source/binary/display/GART hashes; preserve A26 owner both
on target and host; install new owner at the existing bus_managers path and
reboot. Do not try to replace an already resident B_KEEP_LOADED owner in memory.
Bridge lives under drivers/bin/intel_gt_ack_bridge; drivers/dev/misc/intel_gt_ack
links to ../../bin/intel_gt_ack_bridge. Client opens /dev/misc/intel_gt_ack and
makes two synchronous calls. No user BDF/lease/address/packet input is accepted.
Each call retains its private owner pin until final PCI/state recheck and never
copies a partial result on ordinary failure. Invariant failure retains the pin.
Raw CPU fault/hang is not caught; user alarm cannot interrupt stuck kernel MMIO.
Use canonical locks and visible NanoKVM observation; Dan supplies physical reset
if needed because no NanoKVM ATX/reset tool is exposed.

Run build/gt-ack-native-client visibly, then existing paired allocation and PCI
fixtures, and repeat ACK observation. Record source/binary identity, new boot,
raw values/statuses, process/display state and unchanged display/GART artifacts.
Do not infer bit ownership or working forcewake from zero/equal ACK samples.
Research/design limits: research/n100-submission/GT-ACK-OBSERVATION.txt.
