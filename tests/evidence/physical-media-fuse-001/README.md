# A31 physical media-fuse inventory — 2026-11-07

One guarded observation on Intel N100 8086:46d1 revision 00, RenkuOS
hrev60072+518, isolated USB `/dev/disk/usb/0/0/1`. Source: patches 0001–0093.

Candidate identities verified before/after installation and fresh boot:

* Owner v5: `ee8bae824efc37602aad1c8886d39caca55a0c3ea78856122e73935b4aa523fc`
* Bridge: `cb16a3b3b894f02b21bb304d07a0ea65dddeffdba59ba32eafe8fb9751bfbf8e`
* Client: `4d85c30bdadb840f9c8fb5c1d6ace57a525c66ec32650a0b3cdace5abfd1b581`

Native tests passed: 71 actual-owner mocks, 4,101 domain-planner cases,
84 forcewake cases and 37 BCS sampler cases; exit 0. Kernel artifacts are ELF64
x86-64 DYN with NEEDED `_KERNEL_` and required exports; userspace client has
libstdc++/libgcc_s/libroot dependencies. Installed hashes were unchanged by builds.

Physical result (`physical/sample.log`, exit 0):

```text
version=1 module=0 transaction=0 error=0 cleanup=0 stage=6 ownership=5
ACK=0/0x1/0 reads=11 writes=2 elapsed_us=494
media_disable[9140]=0xe00fe planned_domains=4
domain=0 request=0xa188 ack=0x130044
domain=1 request=0xa278 ack=0xd84
domain=2 request=0xa540 ack=0xd50
domain=4 request=0xa560 ack=0xd70
```

The selected domains are GT/render/VDBOX0/VEBOX0; VDBOX2 is fused off. This is
raw fuse evidence plus source-derived decoding, not acquisition of those domains,
engine reset, DMA or GPU command execution.

Postchecks: ACK clear, two PCI lease cycles in D0 and ten paired-GART rounds
(twenty allocations/frees), exit 0. Kernel/display/GART/old BCS bridge hashes
matched the preserved baseline. SSH, app_server and the visible desktop remained
responsive. A31 remains installed; six A29 rollback files are preserved on USB
and workstation. The physical lease was released after evidence collection.

Reboot's 60-second SSH observer timed out, but KVM showed a responsive desktop.
Outgoing pings from the target restored SSH; no interface reset, hardware reset
or rollback was needed. The network cause remains unproven.

Full archive SHA-256:
`4208104279778bda33e6e6a31c462c21dd93a9c3f6f94f01f40f3f89ec0e2c02`
(`build/a31-trial-001/physical-results.tar.gz`). Fresh-boot, native, static
artifact, actual-owner host and physical records are copied alongside this file.
