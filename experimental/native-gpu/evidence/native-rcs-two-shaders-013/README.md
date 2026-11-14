# Bounded second Intel fragment shader, VM checkpoint 013

Date: 2026-11-15 (Asia/Tokyo). Branch: `codex/i915-native-adaptation`.

Mesa 26.3.0 compiled the existing orange fragment shader and a half-green
variant to 208-byte N100 machine programs. The prior compiler proof recorded
identical program metadata and only four differing bytes, at offsets 30–31
and 158–159: both embedded green constants change from `0x3e800000` (0.25)
to `0x3f000000` (0.5). The half-green program SHA-256 is
`ea9fadfaeaa943979f55e30be1b48dc0732b041061699819d34f2bca2e7ef2aa`.

The native owner now admits exactly those two shader BO patterns, including
zero padding across the rest of the 4 KiB page. A one-word mismatch is
rejected. It selects `0xffff4000` or `0xffff8000` as the expected BGRA pixel
for its existing completion inspection. Default RCS draws retain the original
color. The batch packet and shader address patch are unchanged; arbitrary
Intel machine code remains rejected.

The public shader smoke first failed at exit 14 because the second exact
pattern was rejected. After adding its bounded check, the shader, vertex,
and reuse smoke tests passed. The modified owner was then built in an isolated
Haiku VM copy of the known-good physical `native-rcs-combined-015` source and
the matching owner SDK. Both VM and N100 report `hrev60072+518`.
`build-execution.sh` compiled and linked the owner,
then passed its full CPU/fake-engine smoke suite. The VM-only owner binary
SHA-256 is
`fa5b66777e1636892f4fc81a20d5255fdc6f190b1be3b35a3f5f5583b265cd0f`.

No modified owner was installed on the physical N100. A half-green hardware
draw and its readback remain required before this code can replace the
known-good owner. The physical boot image is being preserved and its backing
storage capacity is documented in
[checkpoint 008](../native-ramfs-unmount-008/README.md).
