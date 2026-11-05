# ELAN touchpad investigation

The laptop's Fedora kernel log identifies `ELAN0000` on I²C controller
`8086:54c6`, with ELAN module ID `0x0109` and firmware `0x0002`. RenkuOS
hrev60072+518 detects the controller and touchpad, reports a 3282 by 1793
coordinate range, publishes `/dev/input/mouse/i2c/0`, and starts an `I2C Mouse`
input-server watcher. The user reports no touchpad movement; a USB mouse works.
A short concurrent diagnostic read saw no reports, but it shared the device
with the input server, so that alone does not establish the cause. The system
log includes unexpected report IDs `0x00` and `0x21`; their significance is not
established.

The laptop's archived ACPI SSDT contains the `ELAN0000` device under I2C5. Its
resource template has an extended interrupt descriptor with GSI 100 and flags
`0x1d`, rather than a GPIO interrupt descriptor. The physical RenkuOS log says
the I/O APIC has GSIs 0–119. The `InputReadyLine` implementation in the checked
RenkuOS source handles ACPI GPIO interrupt resources; it does not recognize an
extended interrupt resource. The physical driver consequently reports that it
is polling. This mismatch is a plausible lead, not proof that it causes the
missing touch input. The relationship between the invalid reports and polling
also requires investigation.

Any interrupt implementation must handle ACPI trigger/polarity and sharing,
avoid a level-triggered interrupt storm and lost reports, manage installation
and removal races, and preserve fallback behavior. Do not install an
experimental kernel driver on the laptop until its design and recovery path
have been checked and the relevant hardware test is authorized. Keep the USB
mouse available. Source reference: `src/add-ons/kernel/drivers/input/i2c_hid/
InputReadyLine.cpp` and `src/add-ons/kernel/drivers/input/i2c_elan/` at source
commit `4799bb5a188c4bb6bbef967287640cc21a061a0d`.
