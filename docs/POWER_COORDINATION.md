<!-- SPDX-License-Identifier: MIT -->
# GPU requirements for power-management coordination

Astra High #2 owns graphics; Astra High #1 owns the new power project. This is
a handoff of requirements, not an implemented suspend/resume contract.

The private Intel patches have not executed on the N100 GPU. User-space hook
tests and ordinary-RAM PTE tests cannot establish DMA quiescence or resume
safety. Suppressing legacy 2D hooks does not stop inherited ring initialization.

Before entering a device/system sleep state with a future native GPU backend:

1. Prevent new submissions and synchronize clients with the transition.
2. Drain or explicitly fail outstanding work with a bounded wait. Keep buffers
   pinned until engines can no longer access them; CPU inactivity is insufficient.
3. Mask device interrupts and synchronize active handlers before releasing state.
4. Record ownership of scanout buffers, GGTT entries, engine contexts and PCI
   state. Preserve firmware/ACPI-owned registers and stolen-memory subregions.
5. Coordinate GT and display power domains; restoring PCI configuration alone
   does not restore the display engine, power wells, links or GPU contexts.
6. On resume/reset, restore the owned mappings and required cache/TLB state,
   then contexts, interrupts and display in a validated hardware-specific order.
   Report lost contexts and failed work truthfully; do not signal fake completion.
7. Reopen submission only after validation. A failed resume needs a documented
   recovery path and diagnostic capture, not an unconditional success return.

No GPU suspend/resume support should be advertised from the current patch set.
The inherited local display path, firmware scanout ownership, modern engine
initialization and recovery remain unaudited on physical hardware. Coordinate
shared ACPI/PCIe interface changes before implementation. No shared power APIs
or Fedora graphics settings were changed by this GPU work.

The emulated RenkuOS GPU test VM uses QEMU std VGA and no PCI passthrough. Its
clean shutdown proves test-environment cleanup only, not Intel GPU power control.
