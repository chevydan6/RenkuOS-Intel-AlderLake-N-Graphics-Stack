# Physical A29 BCS inventory — 2026-11-07

One guarded sample on Intel N100 8086:46d1 revision 00, RenkuOS hrev60072+518,
booted from `/dev/disk/usb/0/0/1`. The native candidate owner SHA-256 is
`d9373c865d14b56adaf487d371559d7a3a31ce602d56d6567e64daf094d57217`.
Bridge: `7f2eb4d966147e0dbe5fa7f66544d73fd6d007c7eb553e872d6ff8be262ea982`.
Client: `0ce52461d3a6b5dfcfee67441ac7a481c2cac7e28c8c09d0de03f616d5686607`.

`sample.log` and `sample.exit`: success, Complete/Released, ACK 0→1→0,
two request writes, 480 us. Five raw BCS observations are sequential, not an
atomic snapshot or proof of reset readiness, DMA or command execution.

`postchecks.log`: clear raw ACK observations, PCI lease checks, ten paired-client
memory rounds (twenty allocations/frees); exit 0. `postcheck-system.log` records
unchanged display/GART hashes and the installed A29 owner. Fresh-boot inspection
records loaded paths, kernel, USB medium and app_server. KVM visually confirmed
the desktop and returned Terminal prompt after postchecks.

SSH initially failed after reboot despite a responsive desktop. Console Wi-Fi
down/up/auto-config and outgoing pings restored access before sampling. The saved
pre-reconnect syslog and full archive remain under `build/a29-trial-001/` and on
the target. No physical reset or A28 rollback was needed. The exact network
driver fault was not established by these observations.

The lease record shows exclusive canonical KVM ownership and release after the
transaction. A28 and display/GART recovery copies remain on USB and workstation.
No credentials, whole disk images or candidate binaries are included here.
