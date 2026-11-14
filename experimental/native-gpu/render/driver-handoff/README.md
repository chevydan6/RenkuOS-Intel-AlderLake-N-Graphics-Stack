# Private N100 display-profile handoff candidate

This prepares a new graphics-driver candidate directory. The reviewed baseline
`intel_extreme.cpp` SHA256 is
`e32ce739f36af66dea0f2c2023453549be5b875c32a81c70a62dabfbb47adcb1`.
Its license and original code remain intact. The only source additions are the
private header include and a callback after successful `finish_display(lease,
true)` and `memory_owner_ready = true`.

The baseline only acquires `memory_owner` for vendor `8086`, device `46d1`,
revision zero. The callback additionally requires the ALD device group. It loads
`bus_managers/intel_memory_owner/native_render/v1`, acknowledges the real display
lease using `NATIVE_GPU_RENDER_PROFILE_N100_DISPLAY_ONLY_V1`, and releases the
module reference. The owner validates the active lease and generation; this is
not a userspace flag. Absent export, absent callback, or rejected acknowledgement
logs a diagnostic and leaves display initialization successful. Old drivers never
acknowledge and therefore cannot enable RCS. Owner release handles withdrawal and
drain; no driver teardown change is made.

From `experimental/native-gpu` on the preparation host:

```sh
python3 render/driver-handoff/test-handoff.py \
  --source /tmp/rcs-native-display-001/intel_extreme.cpp
python3 render/driver-handoff/prepare-driver.py \
  --source /tmp/rcs-native-display-001/intel_extreme.cpp \
  --output /tmp/rcs-driver-handoff-candidate-001
python3 render/driver-handoff/check-handoff.py \
  --source /tmp/rcs-native-display-001/intel_extreme.cpp \
  --candidate /tmp/rcs-driver-handoff-candidate-001/intel_extreme.cpp
```

Use a fresh output path for each preparation. The directory contains patched and
original source, both private headers copied from the repository, the exact diff,
a manifest, checksums, and a candidate-only build script. The focused host check
first observes failure for the original source, rejects unrelated edits, and
compiles the actual injected block with module API stubs to check fallback and
balanced module references.

After transferring this directory to a native Haiku build workspace, run from
inside that directory:

```sh
bash build-driver.sh /boot/home/i915-mapping-002/source
```

The native recipe checks its prepared inputs and the original native CPP hash,
then builds `build/intel_extreme` from
the patched CPP plus native `bios.cpp`, `driver.cpp`, `device.cpp`, `power.cpp`,
`kernel_cpp.cpp`, and graphicscommon `memory_manager.c`. It uses the SDK paths and
kernel flags from `build-execution.sh`; native driver/common includes and logging
flags follow the existing native driver Makefile. `--no-undefined`, `_KERNEL_`,
and `haiku_version_glue.o` match that recipe. The Jam target was confirmed against
local `renku-source` HEAD and the existing cycle-89 native build bundle. MTRR is
left to the original driver/source and SDK kernel exports; no new MTRR library or
import is invented. A missing export stops the link for review.

The output includes binary hash, native source hashes, imports, and exports for
review. This script does not install, stage, reboot, or modify any native source,
accelerant, display timing, register access, or teardown code.

Native candidate `003` successfully built with the actual SDK, `_KERNEL_`, and
`--no-undefined`; build evidence is
`/boot/home/native-rcs-driver-003/build.log`. This establishes native compilation
for that candidate. Hardware bootstrap and shader execution subsequently passed; see
`../../evidence/native-rcs-003/README.md` for the handoff and exact limitations.

Before coordinated installation, query the loaded graphics-driver path from the
loader log, resolve its symlink, and record the target's full SHA256. Do not infer
that a system non-packaged copy is active. The first probe returned
`owner_granted=0` because the loader selected
`/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/graphics/intel_extreme`,
whose unchanged `../../bin/intel_extreme` link resolved to an older user-config
binary (SHA256 prefix `ed7cab`), shadowing the system non-packaged candidate
(prefix `74ab`). Those prefixes identify the observed copies; retain full hashes
in installation evidence.

Coordinate the owner, bridge, and actual preferred graphics-driver candidate as
one reviewed change under the lease/reboot workflow. Back up each replaced target
and record its hash and path before an atomic replacement, preserving the driver
link. For the observed preferred graphics target, the backup is
`/boot/home/native-rcs-owner-002/previous-preferred-graphics-driver`. That target
was atomically updated and the coordinated reboot granted the native handoff.
Future installations must recheck the loader-selected path, binary hash, display
usability and owner handoff before claiming hardware validation.

The preparation and focused checks require Python on the host. The native build
uses existing SDK tools only; candidate checksum validation pins the exact source
and original bytes already checked on the host.
