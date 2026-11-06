# Driver staging priority in the physical USB test

The second isolated USB image contained the candidate Intel kernel driver in
`/boot/system/non-packaged/add-ons/kernel/drivers/bin` and a matching legacy
`drivers/dev/graphics/intel_extreme` link. The user booted it on the N100. The
kernel log showed both the system non-packaged and packaged legacy driver
being loaded, but the active kernel image list showed the packaged driver.
The non-packaged GART module and non-packaged accelerant were active. The
packaged driver's legacy clock-gating and overlay messages corroborate that the
candidate kernel driver was not the active graphics device. The internal panel
remained visible; the connected NanoKVM reported no HDMI signal.

The checked RenkuOS source's legacy-driver `get_priority()` checks the broad
`B_SYSTEM_DIRECTORY` prefix before `B_SYSTEM_NONPACKAGED_DIRECTORY`. A
`/boot/system/non-packaged` driver path therefore matches the system prefix
first and is assigned the same priority as the packaged driver. A later
packaged-driver observation can replace the non-packaged path. This is a
source-level explanation consistent with the physical logs; other timing or
selection details have not been ruled out.

A third USB image places the candidate kernel driver and legacy device link in
`/boot/home/config/non-packaged/add-ons/kernel/drivers`, which falls under the
higher-priority user directory. The GART module and accelerant remain in system
non-packaged paths. The candidate device link and binary hash were checked, and
a VM boot with an internal disk also attached verified the USB boot path. The
VM's emulated GPU cannot establish which Intel driver would win on physical
hardware. A physical boot and image-list check remain necessary.
