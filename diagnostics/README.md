# Native boot graphics inventory

This C++ userspace diagnostic gathers prerequisites for a later boot GGTT
ownership analysis. It does not install the experimental Intel driver, map raw
physical memory, read GGTT entries, change display modes or submit GPU commands.
It requires root; the PCI mode uses the existing `/dev/misc/poke` driver.
`--boot-info` uses the separate private `boot_graphics` diagnostic described
below. Neither mode operates the Intel graphics driver.

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
path restores IOPL. The program issues no port operations. The PCI/metadata
modes use inherited interfaces without changing them. Those interfaces were
reviewed at RenkuOS `4799bb5a188c4bb6bbef967287640cc21a061a0d`:

- `headers/private/drivers/poke.h` and `src/add-ons/kernel/drivers/misc/poke.cpp`
- `headers/private/graphics/vesa/vesa_info.h`
- `src/add-ons/kernel/drivers/graphics/{vesa,framebuffer}/device.cpp`

The live physical framebuffer address remains unavailable. The inherited `poke`
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

## Immutable boot snapshot

`boot_graphics.cpp` is a private diagnostic driver publishing
`/dev/misc/boot_graphics`. It reads the existing `FRAME_BUFFER_BOOT_INFO` boot
item at initialization and copies selected numeric fields into a fixed 48-byte
versioned structure. It never exports kernel virtual pointers, maps memory,
attaches to PCI hardware, allocates a framebuffer or changes a display mode.
Its only ioctl checks effective UID, exact buffer size and userspace address
range before a fault-checked copy. Stream reads/writes and other ioctls fail.
The snapshot has no dynamically allocated backing or device resource lifetime.

`--boot-info` reports the firmware boot physical base, visible byte count,
dimensions, depth and stride. The visible byte count is stride × height, not
an allocation-size guarantee. `BootFrameBufferInfo.h` validates signed input,
minimum stride and physical-end overflow before publishing initialized fields.
The private ABI is in `BootGraphicsDriver.h`; it does not alter existing
graphics-driver interfaces and is not proposed as a production graphics API.

This is explicitly **boot-time data**. A subsequent mode change can make it
stale, and even matching dimensions do not prove the same framebuffer address.
It does not establish current scanout, GGTT translation or memory ownership.
The address may identify a PCI aperture rather than its backing memory.

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
builds the userspace diagnostic and private kernel diagnostic, checks invalid
CLI rejection, verifies the QEMU non-target exit and compares three firmware
metadata inventories. Only after these exact platform checks, it installs the
diagnostic into the disposable guest's non-packaged driver directory and tests
its real ioctl, malformed requests, repeated opens and unprivileged access.
It refuses to replace an existing file or symlink, removes its files on exit
and relies on the runner's guest shutdown to end any remaining loaded instance.
It is
specific to the runner's i440fx/std-VGA configuration, not a bare-metal script.

On a separately authorized native boot with recovery verified, run the compiled
binary's inventory modes directly and retain output with the OS version and
boot configuration. Installing the private diagnostic on the physical machine
is a separate, as-yet unperformed step. Do not run the VM validation script on
physical hardware. A
future GGTT reader still requires power-state, mapping and access validation;
these commands do not authorize or perform that step.

Finalize a successful run on the host with:

```sh
python3 tools/finalize-boot-inventory.py build/boot-inventory-005 \
  logs/renku-vm-022/validation --transaction logs/renku-vm-022/transaction.json
```

This checks source hashes, both native result counts, guest cleanup, shutdown,
ELF architecture, kernel dependencies, exported driver functions and binary
hashes before writing `result.json`. Use fresh validation directories; it
refuses to overwrite an earlier artifact extraction.
