# HDMI/DDI pipe detection

The physical laptop booted RenkuOS from its internal 8 GiB partition. Its
`intel_extreme` driver recognizes `8086:46d1` but fails in `map_aperture()`;
`framebuffer.accelerant` supplies the desktop. HDMI output and native display
initialization are not working. The existing GART recognition patch for the
observed `8086:461c` / `8086:46d1` pair is a candidate for the first failure,
but no patched driver has been loaded on the laptop and other initialization
problems remain possible.

Static review identified a separate DDI detection bug. After an EDID is read,
`DigitalDisplayInterface::IsConnected()` checks the first pipe and returns
immediately: an unused first pipe is rejected, while a busy first pipe is
accepted even if all three are busy. Patch 13 scans all three and accepts the
EDID-backed output only when an unused pipe is found, unless an earlier check
has already found an enabled pipe associated with that port. A host test
compiles the actual EDID branch with mock registers and covers all eight pipe
occupancy combinations with and without EDID. The unpatched branch failed five
cases; the patched branch passed all 16.

The complete 13-patch bundle `renku-023` compiled in a disposable RenkuOS
hrev60072+518 VM on the workstation. Existing native tests passed, including
185 accelerant hook checks, 22 lifecycle checks, five clock-gating checks,
28 PTE checks and mocked GART fault tests. The VM had an emulated GPU: those
checks did not exercise physical HDMI, program real GPU registers, validate
actual EDID detection, or prove that a hardware modeset is safe. The device's
existing pipe-assignment code can disable an enabled pipe, so preserving the
working firmware display and a practical recovery path are essential before
any physical driver test. The patch was not installed on the laptop.
