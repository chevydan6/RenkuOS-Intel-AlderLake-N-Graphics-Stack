<!-- SPDX-License-Identifier: MIT -->
# A31 fixed media-fuse inventory

The next reset prerequisite is the actual fused media-domain set. This change
adds owner module v5 while keeping v0-v4 structures and entry points unchanged.
The new endpoint reads exactly one 32-bit register, `0x9140`, under the existing
GT primary forcewake transaction. It accepts no address, mask or user data.

Pinned reference Linux `028ef9c96e96197026887c0f092424679298aae8`:
`intel_gt_regs.h:632-634`, `intel_engine_cs.c:751-813` and
`intel_uncore.c:1370-1374` identify the Gen11 media-disable fuse, its bitfields,
Gen12.0 disable polarity and GT forcewake range. ADL-N platform mask and domain
decoding are recorded in `BCS-RESET-DESIGN.md`. Fuse reading does not acquire the
render/media domains or establish that they will acknowledge a wake request.

The owner still gates 8086:46d1/revision 00 and D0/PCI stability, retains its
numeric request and operation pin during access, and bounds forcewake and
sampling. Endpoint-specific allowlists prevent v5 from reading BCS offsets and
v4 from reading the fuse. Sample stays on the kernel stack until forcewake
release and final owner/PCI validation succeed. Any uncertain read/clock/ACK
or ownership change retains the existing failure ledger and blocks new calls.
No new MMIO stores are introduced: only the established `0xa188` owned-bit
enable and release. No reset, DMA, ring or context operation is included.

Host tests now exercise 71 actual owner cases (the 48 existing cases plus
23 fuse-related cases): success, null/mixed arguments, all-ones, failed ACK set
or clear, final owner failure, competing observations, display close, failed
backing cleanup, PCI/UC/ACK loss, clock faults, register allowlists and bounds.
They run the actual modified production functions under ASan/UBSan, not a
separate implementation. Source patch: `0093-intel_extreme-observe-fixed-media-fuse-under-owned-forcewake.patch`.

Native artifacts are the v5 owner candidate, `intel_media_fuse_bridge` and
`media-fuse-native-client`. The client logs raw fuse and the offline domain
plan only after transaction/cleanup/ACK validation. One guarded physical
invocation follows fresh-boot identity checks; preserve the installed A29 owner
and absent bridge/link state before staging. Reuse ACK/PCI/GART postchecks after
successful release. Failed sampling enters evidence/recovery, not repetition.

## Observed result

The native build passed all four test groups (71/4101/84/37), with unchanged
installed baselines, followed by a successful fresh-boot physical sample:
raw fuse **0x000e00fe**, Complete/Released, ACK 0→1→0, two writes, 494 us, exit 0.
This selects GT/render/VDBOX0/VEBOX0; VDBOX2 is fused off. ACK/PCI and ten
paired-GART rounds passed afterward; visible desktop and SSH were responsive.
See `tests/evidence/physical-media-fuse-001/` for identities and captured logs.

The physical inventory prerequisite is completed. Multi-domain acquisition and
release is now being implemented separately; no reset has run on hardware.
