# First isolated physical Intel graphics test

The user booted a separate NanoKVM USB image on the physical N100. SSH confirmed
`/boot` on `/dev/disk/usb/0/0/1`, and the user reported a visible desktop. The
installed internal RenkuOS and Fedora partitions were not modified for the
experiment. The three staged binary hashes matched the native build artifacts.

The kernel image list showed that the patched non-packaged `intel_gart` module
loaded, while the legacy `intel_extreme` graphics kernel driver came from the
packaged device path. The patched non-packaged accelerant loaded in app_server.
The test image did not contain a non-packaged `drivers/dev/graphics` symlink;
the packaged kernel driver therefore remained selected. This is a partial
candidate test, not validation of the complete 13-patch series.

With the patched GART module, initialization passed the previous missing-GART
failure. The physical log showed the expected Intel bridge, a GGTT window at
`0xbf800000`, the 256 MiB aperture at `0xd0000000`, and a successful
`intel_extreme_init`. The accelerant identified VBT port A, read the internal
panel EDID and performed a 1920×1080 native modeset; the laptop screen remained
visible. No GPU workload or hardware acceleration has been demonstrated.

NanoKVM was connected to HDMI, but it reported no video signal. The driver
reported VBT ports F and G. DDI F's DP AUX read returned NACK, while DDI G's DDC
read failed; neither yielded EDID, and only the internal DDI A was considered
connected. The exact external connector mapping and reason for EDID failure
remain unproven. The DDI free-pipe fix cannot resolve missing EDID by itself.

A second isolated image adds the required legacy-driver device symlink. It
passed a VM two-disk boot and file-hash checks; physical testing is pending.
Continue to use the USB image and the existing recovery procedure rather than
replacing the installed driver's files. The first run included the packaged
kernel driver's legacy initialization, so do not treat it as proof of safety
or use its ring submission path.
