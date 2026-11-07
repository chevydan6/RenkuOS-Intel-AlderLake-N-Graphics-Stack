# A33 reset-preparation failure and A32 recovery — 2026-11-07

One guarded physical v7 invocation on N100 8086:46d1/revision 00,
RenkuOS hrev60072+518 on `/dev/disk/usb/0/0/1`. Source patches 0001–0095.
This trial **failed** and establishes no completed hardware engine reset.

Candidate identities:

* Owner: `99c91dc191798daf0d1341a4d97b3c1477d23b0d12676e8b78913d6b3791f668`
* Bridge: `e8839f425ee4df470af6d30d64535077ecddace0deb77118dc5d25bcb77d212c`
* Client: `84f39cec4d719f79333f052b4566e08d0d7622e3adc4bc4e001b2f9f4ebae282`

Fresh 95-patch reconstruction matched source byte-for-byte; 813 payload hashes
passed. Native VM build passed 113 owner / 431 reset / 964 group / 4101 planner /
84 forcewake / 37 inventory cases. Exact VM/physical kernel+SDK hashes and
artifact hashes/ELF/dependencies/exports were verified before staging.
Eight A32 rollback files were verified on USB and workstation.

After reboot, KVM showed a responsive desktop while SSH initially timed out.
Outgoing pings restored SSH. Fresh uptime, USB root, loaded owner path and exact
candidate/kernel hashes were checked before the one guarded sample.

## Observed failure

`physical/sample.log`, client exit 1:

* Bootstrap Complete/Released, ACK 0→1→0, two writes, 490 us.
* Four group domains acquired; group WorkFailed, Work stage, 1738 us.
* Reset Unsafe, stage WaitReady, 11 reads, **3 writes**, **0 reset rounds**,
  elapsed 780 us; last read `0x941c = 0`; last attempted store
  `0x220d0 = 0x00010001`.
* Each group domain remains Held in its ledger with one enable write.
* Raw fuse output remains zero because failure does not publish it.

The three preparation writes were STOP_RING, prefetch disable and reset request.
No GDRST reset write was issued. The helper reached the domain pre-reset read;
its safety validation failed before the first domain reset store. No guessed
clear or group release followed. The driver retained its observation pin.
The exact failed safety predicate is **not recorded by v7 and remains unknown**.
Do not infer a GPU crash, PCI power loss or ACK loss from the generic Unsafe code.
Desktop and SSH remained responsive and the evidence was collected live.

## Recovery

Restored exact A32 owner and removed the newly installed reset bridge/link after
hash verification, then rebooted through KVM. Fresh A32 boot and all preserved
kernel/display/GART/bridge hashes were verified. Recovery postchecks passed:
clear GT ACK, PCI/D0 leases, BCS inventory (tail/head/control 0, mode0x200,
message-idle0x10), and ten paired-GART rounds. Desktop and SSH were responsive.
No physical intervention was needed. Canonical lease was explicitly released.

A32 owner `b063f9f2c52828b953b5d11dfea2cc192e017666d194a8880da0d72751f10951`
is installed again. The failed experiment was not repeated.

Failure archive: `build/a33-trial-001/failed-results.tar.gz`, SHA-256
`cddd6e413c13c2991a5dfc36b2fa2f0b25b5af5687c6c16668fd74b43da83182`.
Recovered archive: `build/a33-trial-001/recovered-results.tar.gz`, SHA-256
`0fa950ec7e1d17486779589fc0d49f0ffa79b2a6097dfa24782bbf472dc4b9a3`.

Next candidate adds retained safety diagnostics without weakening the checks or
changing the reset sequence, so a bounded diagnostic trial can identify the cause.
