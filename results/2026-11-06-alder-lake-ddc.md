# Alder Lake mobile DDC pin investigation

The complete 13-patch candidate was loaded in the third isolated physical USB
test: the user non-packaged graphics kernel driver, non-packaged Intel GART and
accelerant were all present in the image list. Initialization succeeded and
the internal panel's EDID was read. The external ports' EDID reads failed;
NanoKVM reported no HDMI signal. The observed DDI G DDC address was
`0x4005030`, corresponding to VBT pin 4 in the existing ICP mapping. This is
an inference from the code and log, not a direct firmware dump. The exact
physical connector mapping remains unknown. No GPU workload or acceleration
was demonstrated. An earlier kernel panic around a USB media transition was not
captured, and its cause is unknown.

Linux classifies PCI device `8086:46d1` as an Alder Lake-P display subplatform
(Alder Lake-N). Its Alder Lake-P VBT DDC mapping uses firmware values 1 and 2
for GMBUS pins 1 and 2, and values 3 through 6 for GMBUS pins 9 through 12.
The Linux GPIO enumeration starts at zero, and its register expression is
`0x5010 + 4 * gpio`. Thus these correspond to offsets `0x5014`, `0x5018`,
`0x5034`, `0x5038`, `0x503c`, and `0x5040`. The original Renku ICP branch
selects different offsets. The candidate patch limits the new mapping to
`INTEL_MODEL_ALDM`, which covers the supported Alder Lake-P/N mobile IDs.

Sources: Linux [device classification](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_display_device.c),
[VBT mapping](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_bios.c),
[VBT values](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_vbt_defs.h),
[GPIO enumeration](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_gmbus.c), and
[register formula](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/i915/display/intel_gmbus_regs.h).

The host test compiles the actual `_DDCPin()` function with mocks. It passes
13 checks with the candidate, including absent VBT and unchanged non-ALDM ICP
behavior. The original function fails nine mapping/invalid-input cases.
This is a software-only check: native compilation and a controlled physical
test are still required. It does not establish that the target connector uses
DDI G or that this change fixes HDMI. No additional physical hardware access
was performed during this investigation.

Follow-up: bundle `renku-024` compiled in a disposable RenkuOS hrev60072+518
VM with GCC 13.3.0. The native component tests and host finalizer passed; the VM
shut down cleanly. The candidate accelerant SHA-256 is
`7facc545bc3ad34ad3a87cfb3df4d913c78a5e48d459963ed71cc399dba29fe9`.
The VM uses emulated graphics and did not load or test the Intel GPU.

Physical USB test 004: the running accelerant matched the native build hash,
and the candidate kernel driver and GART were loaded. The driver read EDID
successfully from DDI G at `0x4005038` and listed it as a connected second
port; previously the read at `0x4005030` failed. The log reports DDI G
enabled. These log entries alone are not evidence of a usable picture or of
2D or 3D GPU acceleration.

Correction: the user clarified that NanoKVM is not showing a picture. An
earlier interpretation of their reply as confirmation of an HDMI desktop was
incorrect. `screenmode` reported 1920×1080, 24 bits and 60 Hz; the display log
shows 1920×1080 programmed on DDI G, but a usable HDMI picture is unconfirmed.
NanoKVM status showed `state=0`, `now_fps=0`; the meaning of those values is
not independently established here. SSH remained available after approximately
21 minutes of uptime. Long-term stability and GPU acceleration are unproven.

Offline follow-up lead: the Gen12 port-clock selection code in `Ports.cpp`
uses `config &= TGL_DPCLKA_DDIB_CLOCK_SELECT` before setting the selection,
which appears to preserve the selection mask while clearing unrelated bits.
The log shows the clock configuration changing from `0x1` to `0x4` during
DDI G setup. This requires verification against register definitions and the
platform programming sequence; it is not yet an established cause or a
validated change. No additional hardware registers were written for this lead.

The test image's existing `ScreenSaver_settings` could not be unflattened as
a BMessage. It was backed up as `ScreenSaver_settings.before-gpu-test`, then
a valid settings message with `timeflags=0` was installed and read back. This
disables screen saver and DPMS blanking for the test image; it does not prove
that system suspend or firmware lid behavior has been disabled.
