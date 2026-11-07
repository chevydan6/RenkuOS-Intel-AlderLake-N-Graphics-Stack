# A32 physical all-domain handshake — 2026-11-07

One guarded v6 invocation on N100 8086:46d1/revision 00, RenkuOS
hrev60072+518, USB `/dev/disk/usb/0/0/1`; source patches 0001–0094.

* Owner: `b063f9f2c52828b953b5d11dfea2cc192e017666d194a8880da0d72751f10951`
* Bridge: `5ce038b6c5974d23ce196b7459b75a38c5f5dd0951af58892c7f5b15145eec3f`
* Client: `9bccb8acf2eca4c0a729382ac653deff57637bad3aacbe5537d632a9b164680a`

Fresh pinned-source reconstruction matched development source byte-for-byte;
758 payload hashes passed. Actual-owner ASan/UBSan: 92 cases. Local locked VM
native build passed 92 owner, 964 group, 4101 planner, 84 forcewake and 37 inventory
cases, exit 0. VM and physical kernel/SDK hashes matched. Artifact hashes,
ELF64/x86-64 type, kernel-only dependencies and required exports were checked.

The first VM archive stream failed before extraction: the helper's identity
probe consumed stdin. A separately locked transfer with verified archive hash
corrected this; extraction required the failed destination to remain empty.
Native execution used the documented transaction-lock helper.

Seven A31 rollback files were verified on workstation and USB before replacing
the owner. KVM showed the fresh desktop and responsive Terminal. Outgoing pings
preceded restored SSH; fresh uptime, root medium, kernel/candidate hashes and
loaded owner path were verified before the guarded invocation.

Physical result, exit 0 (`physical/sample.log`):

* Fresh fuse `0x000e00fe`; bootstrap Complete/Released, ACK 0→1→0,
  two writes, 473 us.
* Group Complete/error 0, four domains, zero work callbacks, **1165 us**.
* GT (`a188/130044`), render (`a278/d84`), VDBOX0 (`a540/d50`),
  VEBOX0 (`a560/d70`): each Released, ACK 0→1→0, last ACK 0 and two writes.
* VDBOX2 was excluded by the fuse-derived plan. No reset or GPU workload ran.

Postchecks passed: clear GT ACK, two PCI leases in D0 and ten paired-GART rounds.
Kernel/display/GART/previous bridges retained their hashes. Desktop and SSH
remained responsive; hardware lock was explicitly released after collection.
A32/v6 stays installed. A31 rollback is preserved in `build/a32-trial-001/`
and `/boot/home/a32-group-trial-001/recovery-a31/`.

Physical archive SHA-256:
`707eb1fc3d0fe2cfb49d29023cc239cd82dbdf41089d3eef3c94a8f2096945f3`
(`build/a32-trial-001/physical-results.tar.gz`).

This validates the selected-domain handshake in this controlled boot. Engine
reset, submission, acceleration and Vulkan are subsequent work.
