# Physical HDMI test preparation

The user authorized a physical test. To keep the installed RenkuOS and Fedora
systems intact, a separate sparse UEFI/GPT test image was prepared with the
candidate GART module, Intel graphics kernel driver and accelerant in
`/boot/system/non-packaged/add-ons`. Their SHA-256 values are:

- `intel_gart`: `95851ca350ca64027a9805b10dac08b25135ee4acef48fc0cfa22b641326ac56`
- `intel_extreme.driver`: `bf7befb890824b361c1e7a64d4131805d5e3b2a76de114c5c06a101920d39e50`
- `intel_extreme.accelerant`: `3e8d6b11ab19209331b86bb4402f1d9c653718dec3f1fc73d1b61bc7fad4d185`

The image passed a cold boot in a RenkuOS VM. An initial two-disk test exposed
an important error: its EFI partition contained a GRUB configuration that
selected the other disk, so the candidate was not loaded. A new EFI partition
with a distinct filesystem and a direct RenkuOS EFI loader corrected this. A
second VM rehearsal with an emulated internal RenkuOS disk present booted the
USB partition and verified all three candidate file hashes. This demonstrates
image selection in the emulated environment, not hardware driver execution.

The image was copied to NanoKVM as a sparse file. All allocated extents and
stream metadata passed SHA-256 verification after a read-back; untouched holes
were created as zero-filled sparse regions. An additional full logical-image
checksum was stopped to avoid overloading NanoKVM, so it did not complete. The
original NanoKVM media path is saved separately and a lock-held helper restores
it when the temporary session ends. The running internal RenkuOS system was not
modified. A physical boot of this test image has not yet been demonstrated.

The candidate may fail before networking starts or may blank the display.
NanoKVM did not capture the laptop's HDMI output under the existing framebuffer.
An attending person must be able to select the test USB and, if necessary,
power-cycle and select the known-good internal Fedora or RenkuOS entry. Do not
assume a remote-only recovery path exists. Capture the boot device, loaded
module paths and hashes, kernel log, internal display, and HDMI state before
claiming the physical test succeeded.
