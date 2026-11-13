# Patch diff-target audit: final 13 patches

Inspected patch content where retrievable. No application-state or semantic dependency verification.

| Patch | Observed diff targets | Subject |
|---|---|---|
| `patches/0108-module-entry-contexts.patch` | `src/system/kernel/module.cpp` |  |
| `patches/0109-module-unlocked-init.patch` | `src/system/kernel/module.cpp` |  |
| `patches/0110-module-unlocked-uninit.patch` | `src/system/kernel/module.cpp` |  |
| `patches/0111-acpi-callback-custody.patch` | `src/add-ons/kernel/bus_managers/acpi/BusManager.cpp`, `src/add-ons/kernel/bus_managers/acpi/ACPIPrivate.h`, `src/add-ons/kernel/bus_managers/acpi/ACPICAHaiku.cpp` |  |
| `patches/0112-acpi-interrupt-identity.patch` | `src/add-ons/kernel/bus_managers/acpi/ACPICAHaiku.cpp` |  |
| `patches/0113-acpi-interrupt-admission.patch` | `src/add-ons/kernel/bus_managers/acpi/BusManager.cpp`, `src/add-ons/kernel/bus_managers/acpi/ACPIPrivate.h`, `src/add-ons/kernel/bus_managers/acpi/ACPICAHaiku.cpp` |  |
| `patches/0114-acpi-table-admission.patch` | `src/add-ons/kernel/bus_managers/acpi/BusManager.cpp` |  |
| `patches/0115-acpi-dmar-descriptor.patch` | `src/add-ons/kernel/bus_managers/acpi/ACPIPrivate.h`, `src/add-ons/kernel/bus_managers/acpi/BusManager.cpp`, `src/add-ons/kernel/bus_managers/acpi/Module.cpp` |  |
| `patches/0116-efi-acpi-null-rsdp.patch` | `src/system/boot/platform/efi/acpi.cpp` |  |
| `patches/0117-acpi-root-extent.patch` | `src/add-ons/kernel/bus_managers/acpi/ACPICAHaiku.cpp` |  |
| `patches/0118-efi-acpi-root-kind.patch` | `src/system/boot/platform/efi/acpi.cpp` |  |
| `patches/0119-efi-acpi-unaligned-entries.patch` | `src/system/boot/platform/efi/acpi.cpp` |  |
| `patches/0120-efi-acpi-root-length.patch` | `src/system/boot/platform/efi/acpi.cpp` |  |
