<!-- SPDX-License-Identifier: MIT -->
# Native RenkuOS kernel integration experiment

This is a CPU kernel-services test driver, not an Intel GPU driver. It imports
the original Linux drm_mm, rbtree, refcount and ww acquire-context algorithms.
It allocates wired pages using RenkuOS, runs two real kernel workers, and tests
native synchronization, timeouts, cleanup and ioctl errors. No GPU is claimed,
mapped, reset or submitted to. `linuxkpi_native_probe` is not `gpu_exec_probe`.

## Verified VM checkpoint

- RenkuOS nightly build00081, source
  `af22bfb13d758e13a2d2f6ab204bdbc3e4aab42f`.
- Image: [RenkuOS-x86_64-20261112-build00081.iso](https://github.com/RenkuOS/Source/releases/download/nightly/RenkuOS-x86_64-20261112-build00081.iso).
- Image SHA256:
  `3ffc9ff6c3015467079958e2fcd23d54143b11726e738c5d25daac8d4dc4faf2`.
- Guest reports Haiku R1~beta6+development hrev60072+780 x86_64. RenkuOS
  retains the Haiku uname/API identity; this is the actual RenkuOS image.
- QEMU 10.2.2, KVM, two CPUs, 768 MiB, IDE disk, standard VGA and e1000.
  No physical GPU passthrough. Host is Fedora; tests execute in the guest kernel.
- Matching `haiku_devel-r1~beta6_hrev60072_780-1` from Renku; guest GCC
  `15.3.0_2026_10_11-1` and binutils `2.46.1-1` from HaikuPorts.

The .iso is booted as a raw disk image, with a private qcow2 overlay. With a
normally installed QEMU/firmware, the equivalent launch is:

```sh
sha256sum RenkuOS-x86_64-20261112-build00081.iso
qemu-img create -f qcow2 -F raw \
  -b "$PWD/RenkuOS-x86_64-20261112-build00081.iso" renku-i915.qcow2
qemu-system-x86_64 -accel kvm -cpu host -smp 2 -m 768 \
  -drive file=renku-i915.qcow2,format=qcow2,if=ide \
  -netdev user,id=kpi,hostfwd=tcp:127.0.0.1:10026-:22 \
  -device e1000,netdev=kpi,romfile= -vga std -serial file:renku-serial.log
```

The recorded run used locally extracted QEMU/SeaBIOS with `-nodefaults`, explicit
VGA/firmware paths, `-display none` and a local QMP socket instead of a GUI. All
runtime files are outside the repository in `/home/dan/i915-vm`; no host system
packages, original image bytes, native GPU branch or physical hardware changed.
Sessions 001–003 used a remote image cache and failed with ATA timeouts during
package activation; session 004 uses the complete verified local image.

## Build and run

In the guest, choose **Try it out**, open Terminal, and install the SDK:

```sh
pkgman install -y gcc haiku_devel
```

Verify that the devel package matches the running kernel. The image initially
has neither GCC nor `_KERNEL_`; evidence 065 records the failed initial build.
The guest has about 46 MiB free after the recorded installation. Do not substitute
the older cross-source private headers for this guest SDK.

On the host, from `experimental/i915`:

```sh
python3 vm/prepare-bundle.py --output /path/to/share/native.tar.gz
```

Transfer that archive to the guest (HTTP or SSH), extract into a new directory,
then run inside `i915-linuxkpi-native`:

```sh
bash vm/run-native-tests.sh
cat validation/native-test.log
cat validation/probe.log
cat validation/concurrent-a.log validation/concurrent-b.log
cat validation/exit-status
```

The runner verifies every payload file, builds with the native compiler, links
against the actual `_KERNEL_` import library and `haiku_version_glue.o`, installs
only the named test driver, runs eight repetitions and two concurrent clients
with four repetitions each, and removes its installed files. Existing paths
cause failure. Test invocations are serialized by the driver's run semaphore;
each invocation has two genuinely concurrent kernel workers. Driver publication
is asynchronous: a bounded ten-second readiness wait handles reinstall timing.
The expected exit status is 0. Serial INIT/UNINIT tags establish actual module
load/unload; removing a file alone is not unload evidence.

`bash vm/build-native.sh` builds without installing or running. Artifacts:

- `build/native/i915_linuxkpi_test.driver`: real loadable CPU test driver.
- `build/native/linuxkpi_native_probe`: native ioctl test client.
- `build/native/i915_execution_preparation.o`: native-compiled original Linux
  ring/emitter bodies, separate from the installed driver. `nm -u` deliberately
  retains `i915_request_timeline`, `renku_ring_wait_for_space` and `panic`.
- `validation/`: actual results, compiler/kernel records, imports and hashes.

Final recorded results are in `../evidence/native-vm-006/`: 16 × 62 checks pass,
64 wired pages allocated/freed, 32 kernel workers, 64000 iterations through each
lock/reference path, and measured 25060–25284 microsecond timeouts. No benchmark
claim follows from these timings.

The first loaded version panicked with B_BAD_THREAD_ID after a worker exited
(066). RenkuOS `thread.cpp` at the image's exact source revision shows that
kernel threads leave no cached death record. The corrected late-join branch
requires a received completion and an absent native thread ID; other errors
remain fatal. An early reinstall also failed before devfs publication (067).
These failures are retained. The final checks were extended with repeated
cleanup, deleted semaphore operations and timeout-overflow rejection.

The serial log's three `Bad address` faults at address zero are the deliberate
invalid-user-buffer ioctl tests, handled by real `user_memcpy`; the client
checks error returns and the VM continues.

## Boundary before a physical trial

The compiled execution object still has private slice request/ring layouts.
It cannot accept a full Linux i915 request. The wired-page backend establishes
CPU physical addresses only: it does not establish device DMA addresses or
GGTT/PPGTT mappings and must not be used as an IOMMU bypass.

Next integrate those owned pages with the native branch's demonstrated memory
mechanisms, real GPU virtual mappings, GT initialization, context and submission
ownership, then a real completion path and ring retirement. Keep backing alive
until hardware retirement/reset is established. Only then implement
`gpu_exec_probe --store 0x91501234 --timeout-ms 1000` against that driver ABI.
There is no success-returning GPU ioctl or CPU-written fake result here. The
existing CPU probe can preflight native services in a later physical test,
but its PASS says nothing about GPU execution.

## Physical N100 run

The exact native-006 source bundle also passed on the live four-core Intel N100
with GPU 8086:46d1 and bridge 8086:461c. The machine runs hrev60072+518 from the
external RenkuTest USB volume, with matching haiku_devel and GCC13.3. It compiled
its own driver/client/execution object; no newer VM binary was installed.

Actual build/tests were launched through NanoKVM keyboard into fullscreen
Terminal. `evidence/physical-n100-001/run-visible.sh` records the wrapper, which
follows real build output and prints all client results. SSH handles transfers
and evidence collection. The identical suite passes 16 × 62 checks; timeouts
measured 25003–25023 us. Native syslog confirms INIT/UNINIT status=0 and strict
postchecks confirm removal of the test driver/device. Intel/display drivers
and the attached boot image were preserved, and no GPU work was submitted.

Reviewed results are under `../evidence/physical-n100-001`. Raw archives and
screenshots remain in `/home/dan/i915-vm/evidence`, with local binaries also in
`../build/physical-n100-001`. The physical machine is left running RenkuOS.
