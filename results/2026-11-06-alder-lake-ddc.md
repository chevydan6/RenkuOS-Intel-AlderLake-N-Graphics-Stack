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
