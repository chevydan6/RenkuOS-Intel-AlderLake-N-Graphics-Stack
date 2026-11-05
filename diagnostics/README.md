# Native boot graphics inventory

This C++ userspace diagnostic gathers prerequisites for a later boot GGTT
ownership analysis. It does not install the experimental Intel driver, map raw
physical memory, read GGTT entries, change display modes or submit GPU commands.
It requires root; the PCI mode uses the existing `/dev/misc/poke` driver.

`--pci` reads identity at fixed BDFs `00:00.0` and `00:02.0`. It exits 3 on
anything other than the verified `8086:461c` / `8086:46d1` pair, before reading
the target-specific register list. On that pair it reports raw GGC, stolen
bases, BARs, command and revision/class fields. No BAR sizing writes are used.
Values are observations; a matching PCI ID does not establish readiness for
MMIO or command submission.

`--framebuffer /dev/graphics/vesa` or
`--framebuffer /dev/graphics/framebuffer` queries the already active firmware
graphics device on a running desktop. Use the correct existing device; opening
an inactive driver could trigger its first-open initialization. The diagnostic
verifies its accelerant signature, clones only metadata read-only, reports
dimensions and stride, then releases the clone and file descriptor. It does
not clone or access pixel memory. This mode does not open `poke`.

Opening `poke` temporarily raises the calling thread's IOPL on x86; its close
path restores IOPL. The program issues no port operations. No new kernel ABI
or driver is introduced. The interface definitions and implementation were
reviewed at RenkuOS `4799bb5a188c4bb6bbef967287640cc21a061a0d`:

- `headers/private/drivers/poke.h` and `src/add-ons/kernel/drivers/misc/poke.cpp`
- `headers/private/graphics/vesa/vesa_info.h`
- `src/add-ons/kernel/drivers/graphics/{vesa,framebuffer}/device.cpp`

The physical framebuffer address remains unavailable. The inherited `poke`
physical-address query calls `get_memory_map`, which can panic if a mode change
invalidates the framebuffer clone before translation. A prototype using this
query was removed after auditing `src/system/kernel/vm/vm.cpp`, before any
native execution. The existing accelerant hook returns NULL for
`frame_buffer_dma`, so it cannot supply this address either. A suitable query
must coordinate with the graphics driver's mapping lifetime.

Even a physical framebuffer address can be a PCI aperture address rather than
the backing page for a GPU virtual address. The tool does not infer a GGTT
offset or firmware ownership. Metadata reads can race a mode change; keep the
desktop mode fixed and compare repeated captures.

## Build and VM validation

Prepare a fresh bundle from the local pinned source:

```sh
python3 tools/prepare-boot-inventory.py \
  --source ../source/gpu-worktree --output build/boot-inventory-001
tar -C build -czf build/boot-inventory-001.tar.gz boot-inventory-001
```

Pass this archive to the existing `tools/renku-vm-test.py` under the canonical
Fedora host-config/haiku-vm/build locks, with the stopped RenkuOS test source
disk, verified guest SSH configuration and a fresh output directory. The
runner preserves the original VM, checks space, clones the disk, captures
logs and shuts the disposable VM down. `tools/run-native.sh` inside the bundle
builds only this diagnostic, checks invalid CLI rejection, verifies the QEMU
non-target exit and compares three firmware-framebuffer inventories. It is
specific to the runner's i440fx/std-VGA configuration, not a bare-metal script.

On a separately authorized native boot with recovery verified, run the compiled
binary's two modes directly and retain output with the OS version and boot
configuration. Do not run the VM validation script on physical hardware. A
future GGTT reader still requires power-state, mapping and access validation;
these commands do not authorize or perform that step.
