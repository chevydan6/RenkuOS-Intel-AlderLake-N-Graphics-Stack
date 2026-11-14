<!-- SPDX-License-Identifier: MIT -->
# Native Intel N100 GPU handoff — 2026-11-14

## Separate render-engine phase: shaders, OpenGL and Vulkan

The [Mesa source-tree guide and render-phase boundary](research/n100-submission/MESA-RENDER-ENGINE-PHASE.md)
now records what carries forward from BCS and what remains render-specific:
owned RCS contexts/state, actual topology/capabilities, Intel shader compilation,
general buffer/GPU-VA interfaces, render/cache and cross-engine synchronization,
and frontend/presentation integration. BCS context/status helpers are not generic
RCS helpers. Preserve their working hardware logic while adding render support.

Mesa Iris, compiler/ISL/genxml, Haiku hgl/EGL and ANV are reference/reuse targets,
not ready-made native integrations. The guide uses checksum-verified Mesa 26.0.7
for Iris and the modern Haiku software EGL path, and separately labels classic
Mesa 22.0.5 BGLView/renderer-add-on references. The checked 26.0.7 Haiku EGL path
constructs a software screen; it does not select Iris. Iris's KMD backend is
only part of its DRM/OS dependency surface. Vulkan/ANV and native WSI need their
own integration after shared native contracts exist.

The first render proof should be a compiled shader draw to owned offscreen
memory with verified output and completion, not a version string, clear or BCS
copy. No Mesa build, shader compilation, RCS execution or hardware OpenGL/Vulkan
was performed in this investigation. Native BCS binaries remain unchanged.
The source guide includes two release archive checksums and 26 file hashes.
Desktop presentation work can proceed independently; rendering is not its
prerequisite and full LinuxKPI closure remains outside the active strategy.

## Desktop integration investigation and recommendation

[Source investigation and integration decision](research/n100-submission/DESKTOP-INTEGRATION-INVESTIGATION.md)
traces the pinned RenkuOS `4799bb5` path. Normal local-backend drawing targets a
malloc-backed RGBA buffer; damaged rows are copied to the front buffer by CPU.
The local app_server backend does not consume legacy screen-blit hooks, although
BWindowScreen does. Implementing that hook alone is not desktop acceleration.

Keep CPU presentation enabled. The next narrow integration target is batched
presentation from the existing CPU back buffer through persistent GPU staging
to an explicitly leased scanout destination, without CPU destination readback.
Larger owned buffers, batching, visibility/retirement and cursor/mode/direct-client
ordering are required. First measure actual CPU-to-scanout presentation and
representative larger/batched private copies; the current <=4 KiB warm-memory
benchmark does not establish that crossover. A general GPU offscreen/bitmap
interface is broader follow-on work, not the first required integration.

The report records 21 inspected source blob hashes and identifies concrete
entry points and missing contracts. This was read-only investigation: no desktop,
accelerant or kernel code was changed and no new hardware trial was performed.
Installed binaries and the latest native benchmark checkpoint remain unchanged.

## Latest checkpoint: native demonstration and CPU/GPU benchmark

A native BeAPI window now displays a32×32 source tile, verified BCS GPU readback,
and timing results. [Benchmark report, CSV, screenshot and exact evidence](experimental/native-gpu/evidence/native-benchmark-003/README.md)
cover64,256,1024and4096-byte copies. All484requests passed in one boot.

The prototype is slower than warm CPU memcpy throughout this range: the complete
GPU copy path takes about2.66–2.68ms, versus10–89ns for CPU copies. Upload/copy/readback
also takes about2.67–2.68ms. Fresh-boot session open/create took97.5ms and first
copy15.3ms. Measurements include staging, cache maintenance, submission, polling,
verification and retirement; they do not isolate intrinsic engine time. No larger
crossover or BCS throughput claim is possible with the current4KiB buffer limit.

Two initial runs stopped at bootstrap; diagnostic evidence identified the PCI
stability callback (`WaitSet/PowerChanged`). Bootstrap/inspection now uses the
same native access/power anchor policy as group work, preserving mapping/lease
checks, full initial admission, legacy diagnostics and Intel hardware sequences.
The exact differing snapshot field was not captured. See the report for details.

Current installed owner SHA256:
`dbb36045d998c2440f7c32bd82572946d3ca014bb6a852004c87d614251f841d`.
The v2 bridge is unchanged from the buffer-interface checkpoint. Native source/
build/client: `/boot/home/native-benchmark-owner-003`. Run `build/gpu_copy_bench`
in a fresh output directory, with `--show` for the native window. The demo window
was captured then closed normally; app_server remains PID127, original component
hashes match, and the physical lease is released. Further successful copies need
no reboot. The observed VT-d-disabled restriction remains; general IOMMU support
is separate work.

## Latest checkpoint: owned-buffer rectangular copy interface

The narrow [buffer-and-copy API](experimental/native-gpu/BUFFER_COPY.md) now runs
on the physical N100. Each open session owns two fixed4KiB CPU staging buffers;
handles, byte ranges, dimensions, pitches, bounds and non-overlap are validated
before publication. All commands and GPU addresses are built in the kernel.
GPU execution uses the resident owner's separate pinned pages; no user/staging
pointer escapes the synchronous call. Visibility and request lifetime rules are
explicit in the API document. Uncertain execution retains GPU backing and blocks
submission until reboot. Direct DMA still requires observed VT-d translation
disabled; general IOMMU support remains separate.

[Native buffer evidence](experimental/native-gpu/evidence/native-buffer-001/README.md):
three17×5 rectangle copies with source/destination pitches80/96 passed in one
boot, including bytewise destination-padding verification and rejection of
same-buffer, out-of-bounds and unowned-session handles. The original4KiB sample
then passed at sequence4. EIR/fault remain0; app_server remains PID127 and the
thirteen original display/observer hashes match. The physical lease is released.

Native source/build/client: `/boot/home/native-buffer-001`. Run
`build/buffer_copy_sample` or `build/gpu_copy_sample`; both may be invoked again
following successful retirement. **Both owner and bridge now use the private
native_execution/v2 module** and must be replaced together, with a reboot.
Current installed owner SHA256:
`c114d975c83536134b153d328fafc47920d719f4c0a2d69df5ac3f256ff7ce14`.
Current installed bridge SHA256:
`30d6283481973cec65cbc58be7922c09df3fe98f9a3fd80d4166e82f89b808c8`.
The older installer remains guarded for the original pre-execution baseline;
it is not an upgrade script for these already-installed versions.

## Latest checkpoint: repeated copies in one boot

The one-attempt-per-boot restriction described by execution005 below is superseded.
Seven sequenced GPU copies now pass in one boot, including CSB wrap and reuse,
with one request in flight and polling. See
[implementation, retirement contract and native evidence](experimental/native-gpu/evidence/native-repeat-003/README.md).

Device reset/cache/TLB/engine initialization and backing allocation run once.
Success requires current-tag CSB activation/switch-away, current ordered sequence,
nonce/complement, all1024 copied words, CPU visibility and idle/error/owner checks,
then successful forcewake release and final owner/DMA checks. Only that complete
path reopens admission. The same context/ring/HWSP/PPGTT/data backing is reused.
Failures retain memory and block all further submissions until reboot.

Current installed owner SHA256:
`79b4a82f60a1dbe8302b21eab3899f1a9753435bda691449a7972890cc9b0261`.
Bridge is unchanged from execution005. Native source/build/client is
`/boot/home/native-repeat-003`; `build/gpu_copy_sample` can be invoked again
without reboot after a success. Resident-code replacement still requires reboot.
The seven requests have retired successfully; app_server remains PID127, thirteen
original display/observer component hashes match, and the physical lease is released.
There is no general buffer deallocation, interrupt scheduling or reset recovery
in this milestone. The remainder of this file preserves historical checkpoints.

## Start here: the owner changed the development strategy

The owner explicitly chose **native adaptation**, allowing changes to integration
code while retaining Intel's hardware logic. The active goal is:

> Native adaptation: allow changes to i915 integration code while retaining
> Intel's hardware logic and get sample code running on RenkuOS that is GPU accelerated.

**Abandon the full Linux i915/LinuxKPI port as the active strategy.** Do not keep
expanding Linux scheduler, VFS, sysfs, per-CPU, timer or memory-manager compatibility
to make unchanged i915 translation units compile. Full original-source compilation
is no longer a prerequisite for this goal. Preserve original imports as reference;
adapt the native integration and hardware routines where needed, retaining source
attribution and licenses. Do not discard the existing validated native services.

The owner also explicitly requested staying on task: **no exhaustive testing,
relentless edge-case testing or unrelated hardening**. Use focused build/smoke
checks, investigate actual failures, and move toward a real GPU-accelerated sample.
GPU-visible ownership and bounded recovery remain necessary implementation work.
Do not replace GPU execution evidence with increasing CPU test counts.

## Current outcome and where the work is saved

**The requested native GPU-accelerated sample now runs on RenkuOS.** On the
physical N100, native execution005 returned ioctl/status0 and copied4096bytes
between two private PPGTT pages. All1024dwords match; both hashes are2bb2fdc5.
Nonce032d377f, complementfcd2c880 and ordered sequence1 match. Fresh CSB activation
and idle-completion records were accepted; BCS mode is0x200, EIR/fault are0, and
the forcewake group completed release (stage6/status0). app_server remains alive.
See [actual GPU copy evidence](https://github.com/chevydan6/haiku-intel-gpu/tree/codex/i915-native-adaptation/experimental/native-gpu/evidence/native-copy-005).

The runnable client is
[gpu_copy_sample.cpp](https://github.com/chevydan6/haiku-intel-gpu/blob/codex/i915-native-adaptation/experimental/native-gpu/gpu_copy_sample.cpp).
This is a real BCS copy workload, not a CPU simulation, a complete 3D/render stack,
or a general reusable submission API. The initial prototype permits one attempt
per boot and deliberately retains its request backing until reboot, including
after success. Power requests were released normally after verified completion.
The earlier native preparation and engine-only compilation checkpoints remain
historical evidence below.

- Active continuation: [codex/i915-native-adaptation](https://github.com/chevydan6/haiku-intel-gpu/tree/codex/i915-native-adaptation).
  This saves the new native code and this handoff, starting from the validated
  checkpoint66b3981. Continue native development there.
- Last physical/native service checkpoint:
  [66b3981](https://github.com/chevydan6/haiku-intel-gpu/commit/66b3981c6947402eeb02928ee5155e63c92cf511).
  The native dirty/accessed bridge passed233828 checks, including its existing
  completion/memory and VMA regressions. This is CPU/kernel evidence, not GPU work.
- Frozen LinuxKPI branch: [experiment/i915-linuxkpi](https://github.com/chevydan6/haiku-intel-gpu/tree/experiment/i915-linuxkpi).
  It remains at66b3981; keep its evidence and implementations available.
- Later, unfinished compiler exploration is preserved separately at
  [866a95ab](https://github.com/chevydan6/haiku-intel-gpu/commit/866a95abd0f2bb05291055afe96858b5f2f2f886),
  branch [archive/i915-linuxkpi-compiler-closure](https://github.com/chevydan6/haiku-intel-gpu/tree/archive/i915-linuxkpi-compiler-closure).
  It includes the declaration/header work, two byte-exact timer imports and all
  new compiler logs. Target-header atomic smoke and component builds passed;
  all four complete original driver sources still fail. Do not resume that as
  the primary development path.

GitHub was checked directly. The pre-handoff default branch gpu/renkuos-n100 was
49a32ff (November13); it contains the existing native context/CSB work. The branch
gpu/native-memory is older, ac2c05a (November7), and does not contain the newer
ContextImage header. Do not assume its name identifies the newest native work.

## Reuse the existing native development

The repository already contains substantive native code. Read these before
creating another implementation:

- [GPU_FIRST_EXECUTION_PLAN.md](GPU_FIRST_EXECUTION_PLAN.md): direct BCS0 ring
  nonce/complement proof, ordered completion, owned context/address space and
  retirement. Its old per-checkpoint exhaustive-test lists are historical;
  the owner's newer focused-testing instruction takes precedence.
- [IntelN100ContextImage.h](workstreams/gpu-next/IntelN100ContextImage.h): full
  16KiB Gen12 BCS context, ppHWSP, register image and workaround pages.
- [IntelN100ContextStatus.h](workstreams/gpu-next/IntelN100ContextStatus.h): BCS
  descriptor and context-status interpretation.
- [IntelN100CsbReader.h](workstreams/gpu-next/IntelN100CsbReader.h): engine HWSP
  initialization and bounded context-status reader.
- [IntelN100Ppgtt.h](workstreams/gpu-next/IntelN100Ppgtt.h) and
  [IntelN100PpgttTree.h](workstreams/gpu-next/IntelN100PpgttTree.h): 4KiB page-table
  encoding and owned scratch/private hierarchy construction.
- [IntelN100NonceStores.h](workstreams/gpu-next/IntelN100NonceStores.h): eight
  dwords for GGTT nonce/complement stores. These are payload only, not a full request.
- Existing IntelN100Forcewake/ForcewakeGroup/BcsReset helpers and the retained
  IntelMemoryOwner module: physical forcewake and reset work already exists.
- [experimental/gen12/LinearCopy.h](experimental/gen12/LinearCopy.h): existing
  linear BLT packet builder for the later accelerated copy sample. Check the
  packet/address/cache profile against the intended N100 path before use.
- [CONTEXT-IMAGE-DESIGN.md](research/n100-submission/CONTEXT-IMAGE-DESIGN.md),
  [CONTEXT-STATUS-DESIGN.md](research/n100-submission/CONTEXT-STATUS-DESIGN.md),
  [CSB-READER-DESIGN.md](research/n100-submission/CSB-READER-DESIGN.md) and
  [EXECUTION-OWNERSHIP.md](research/n100-submission/EXECUTION-OWNERSHIP.md)
  preserve the hardware field/lifetime research.

The six context/CSB/PPGTT/nonce files above were recovered from an existing local
native workspace and verified against their exact committed Git blob IDs. The
ContextImage blob2e58701f was also checked against GitHub's default branch. These
are existing results being reused, not newly implemented or newly hardware-tested.

Useful work from the LinuxKPI phase also carries forward: native owned/pinned
pages, pageable backing ownership, GGTT mapping/alias integration, native DMA
bridge, synchronization, timing and the actual SDK build recipes. Refer to
experimental/i915/{src,include,dma,mapping}; use the native services directly where
appropriate. Linux-only interface scaffolding can stay archived.

## New code saved in this handoff

[experimental/native-gpu](https://github.com/chevydan6/haiku-intel-gpu/tree/codex/i915-native-adaptation/experimental/native-gpu)
on the continuation branch contains the new code:

- RequestImage.h composes the existing context, descriptor, HWSP and nonce helpers
  into a fixed Gen12 BCS0 request image. It adds a stalling MI_FLUSH_DW before the
  completion store, the Gen12 preemption point and WaIdleLiteRestore tail padding.
  Normal tail is96bytes; padded tail is104bytes. The completion word is at engine
  HWSP+0x100; the preemption semaphore at+0xc8 starts zero. Result and completion
  start poisoned. Polling uses MI_NOOP in place of MI_USER_INTERRUPT because the
  native display IRQ handler does not own this request.
- prepare_module.cpp is a **native-built, once-invoked preparation prototype**. It borrows
  the existing memory-only GGTT set, uses the previously validated native page
  allocation bridge to build a nine-page PPGTT/scratch hierarchy, prepares actual
  request/context/status/result backing, orders WC writes before CPU readback,
  and releases unpublished memory. It keeps failure records in a resident module.
- PrepareModule.h and PrepareReport.h define the private preparation-module and
  fixed report interfaces. No user MMIO addresses, command streams or pointers
  are exposed. There is no engine-publication operation in this prototype.
- prepare_bridge.cpp, prepare_client.cpp, build-native.sh and run-native-prepare.sh
  provide target compilation and one kernel call. Native status fields all0;
  tails96/104bytes, result_poisoned=1, gpu_submitted=0. The bridge unloaded,26 source
  hashes and13 production hashes matched, and app_server remained PID127. Actual
  include-order and freestanding-flag build errors were corrected.
- request_image_smoke.cpp performs a focused valid-request check. It compiled
  with host G++ using -std=c++17 -O2 -Wall -Wextra -Werror and passed. The initial
  missing-header build was observed before implementation. No new broad matrix,
  sanitizer/mutation suite is claimed. The native preparation call above is
  owned-memory/kernel evidence and does not demonstrate GPU execution.

Reproduce that one check from the repository root:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Werror -Iworkstreams/gpu-next \
  experimental/native-gpu/request_image_smoke.cpp -o /tmp/native-gpu-image-smoke
/tmp/native-gpu-image-smoke
```

Limits of the preparation-only prototype: rootPhysical is a pinned CPU physical address, not proven GPU
DMA admission. The image uses a fixed N100 profile; hardware execution must obtain
fresh actual identity/stepping and ownership. Existing mapping/v1 is explicitly
memory-only and permits end_set only because it has no GPU publication. Do not
submit these borrowed objects and then reuse its ordinary cleanup. The execution
owner/ABI must retain the complete request until retirement or established reset.

## Sample execution and current prototype limits

Intel documentation inventory: [docs/INTEL_GPU_DOCUMENTATION.md](docs/INTEL_GPU_DOCUMENTATION.md).
Use the N100 research map, the applicable Intel Xe-LP manuals and the N100
paths in pinned open-source drivers. A standalone N100 GPU programming manual
is **not a prerequisite** and must not block native development. Check concrete
platform differences as they arise; do not restart broad documentation searches.

1. The requested GPU copy milestone is proven by native-copy-005. Do not repeat
   CPU preparation or full Linux source-compatibility work as prerequisites.
2. The native owner now integrates fresh PCI/DMA admission, owned PPGTT/GGTT
   backing, forcewake/reset, PAT/MOCS/cache/TLB setup, BCS/HWSP initialization,
   ELSQ publication, ordered completion and CSB interpretation. Intel hardware
   commands/sequences and existing native helpers are retained with attribution.
3. Current execution is fixed and single-use per boot. Backing stays retained;
   another run or replacing resident code requires a controlled reboot. General
   reusable requests and normal backing reclamation are future native driver work,
   not functionality claimed by this sample.
4. This establishes GPU copy-engine acceleration. EU shaders, OpenGL/Vulkan,
   media acceleration and a complete accelerated desktop are not demonstrated.

Keep checks proportional: focused compilation and actual workload verification.
The physical iterations below addressed observed failures; no exhaustive matrix
or unrelated hardening was added.

## Physical target and current operational state

- Intel N100 / Alder Lake-N,8086:46d1 revision0. Physical RenkuOS kernel:
  hrev60072+518, x86_64, native GCC/G++13.3. Preserve the known-working HDMI build057
  and RenkuTest USB boot setup. app_server remained PID127 at the last postcheck.
- Laptop192.168.1.114, SSH2222, username user. That account is UID0. The owner
  explicitly authorized full SSH development/build/test/write access and root
  access; there is no user-imposed transfer-only or read-only SSH restriction.
  Port2222 is a boot-local test listener; default22 has a different root policy.
- NanoKVM http://192.168.1.210 provides visible HDMI and keyboard. Keep native work
  observable in Terminal, for example by following the native build log while
  SSH executes commands. Do not reset/rebind the HID gadget: it shares boot media.
- The battery is removed; power comes through the Bluetooth Govee H5080 outlet.
  Restoring outlet power automatically starts the laptop. The owner authorized
  unattended power recovery after a confirmed hang. Use the existing private
  controller and key; do not duplicate credentials into GitHub.
- Acquire a fresh bounded exclusive lease at
  /root/power-recovery-20261105/physical-test.lock on NanoKVM for physical operations.
  The last lease was released and the appliance shell prompt confirmed. The new
  preparation add-on/bridge were temporarily installed and invoked, then their
  test files removed. That preparation test did not replace production components. The later native
  execution checkpoint intentionally replaces the resident owner through a
  controlled reboot; the display driver and accelerant remain unchanged.
- NanoKVM boot media remains /root/renku-gpu-test-004/disk.img through its current
  USB mass-storage gadget; preserve that selection and the recovery setup.
- Native wall time incorrectly reports October1. Session UTC date is November14.
  Use content/binary hashes and actual runtime evidence, not native wall time.

Local checkout: /home/dan/projects/i915-native-validation, branch
codex/i915-native-adaptation. Pinned reference sources are Linux v6.6
ffc253263a1375a65fa6c9f62a893e9767fbebfa and RenkuOS Source
4799bb5a188c4bb6bbef967287640cc21a061a0d. Older native helpers cite their own
recorded Linux revision; preserve those attributions rather than relabeling them.

The existing native bundle source matching the installed mapping interface is on
the laptop under /boot/home/i915-mapping-002/source. The pre-execution owner
binary SHA256 was0a6ff95a029f125f89fecaab2b899c1346555793b2e620c922837eb27b5a483a.
Its backup is /boot/home/native-gpu-execution-001/previous-intel_memory_owner.
The current native execution owner hash is recorded in the latest checkpoint below. An older gpu-memory-089-001 build has a different hash: do not
replace the current owner with that old binary. Resident B_KEEP_LOADED modules
need a controlled reboot for replacement; file replacement alone changes no
already-loaded code. Preserve and verify the actual installed baseline first.

Native page-state evidence is retained in /boot/home/i915-native-state-001 and
locally /home/dan/projects/i915-native-state-evidence-001. Its published summary
and logs are in experimental/i915/evidence/physical-page-state-001. Private access
helpers are under /home/dan/.local/share/renku-access; Bluetooth control/key is
under /home/dan/.local/share/govee-control. Credentials and key contents are
deliberately absent from this handoff and repository.


## Latest preparation checkpoint operational note

At the historical preparation checkpoint, bus_managers/intel_native_prepare/v1
remained resident under B_KEEP_LOADED after its backing was released and its test
files removed. Subsequent controlled reboots replaced that boot. Do not assume
the old preparation module remains loaded now. Its raw evidence is retained in
/boot/home/native-gpu-prepare-001 and /home/dan/projects/native-gpu-prepare-evidence-001.
The current resident code is the native execution owner described below.


## Parallel Intel documentation research now available

New documentation-only commits5ae4a83 andc02b93e are preserved on the native branch:
[Intel documentation / i915 map](https://github.com/chevydan6/haiku-intel-gpu/blob/codex/i915-native-adaptation/research/n100-submission/INTEL-DOCUMENTATION-I915-MAP.md)
and [N100 UHD architecture/capabilities](https://github.com/chevydan6/haiku-intel-gpu/blob/codex/i915-native-adaptation/research/n100-submission/N100-UHD-GRAPHICS-RESEARCH.md).
Use those exact links/sections for the remaining execution integration instead
of restarting broad research. They are documentation findings, not hardware tests.

The research agent also added a [Tiger Lake PRM / i915 submission comparison](https://github.com/chevydan6/haiku-intel-gpu/blob/codex/i915-native-adaptation/research/n100-submission/TIGER-LAKE-PRM-COMPARISON.md).
Check its platform-applicability notes when wiring the remaining engine path.

## BCS engine setup/publication checkpoint

BcsEngine.h now adapts pinned Linux v6.6 reset_csb_pointers,
enable_execlists, write_desc and execlists_submit_ports into a fixed N100 BCS0
native helper. It programs the engine HWSP and CSB cursors, selects Gen12
execlists mode, clears STOP_RING, clears unused ELSQ port1 and writes port0
low/high before LOAD. Polling leaves engine error interrupts masked. The ledger
records any engine store and an attempted LOAD before calling the backend.
It grants no cleanup permission and does not implement DMA/cache admission or
retirement. The retained execution owner must supply those prerequisites.

The actual RenkuOS kernel SDK compile and one native simulated-IO smoke passed.
No GPU register stores or GPU work were performed by this check. Evidence:
[BCS engine build](https://github.com/chevydan6/haiku-intel-gpu/blob/codex/i915-native-adaptation/experimental/native-gpu/evidence/bcs-engine-001/README.md).
Native source/build: /boot/home/native-bcs-engine-001. Its physical lease was
released after the build. At that checkpoint the next task was connecting the
owned request and this helper inside the retained owner. The verified execution
checkpoint below now implements that connection and proves the copy sample.

## Verified native GPU copy — execution005

Native source/build/client: /boot/home/native-gpu-execution-005.
Run build/gpu_copy_sample from that directory on a fresh boot with the matching
owner/bridge installed. The successful boot's single-use request has already been
consumed; reboot before another invocation. Do not substitute the old v1 prepare
endpoint or expect copying a new owner file to replace resident code.

Current installed owner SHA256:
015fac339a45e8ab28e0d22e74299e477d723a705cbe222cc2df7836f89a1fd8.
Current installed execution bridge SHA256:
a6744f97565c9b1441249365c09ee843d66cdeb1fe9e144a9a04c36916963c6f.
The thirteen original display/observer components still match their recorded
baseline hashes. HDMI build057 and boot media were preserved. app_server PID127
was present after the trial. The physical lease was released; NanoKVM shell
prompt was confirmed. SSH2222 was restored through native Terminal after reboot;
no boot startup script or new credentials were installed.

Raw exact source, objects, binaries and logs:
/home/dan/projects/native-gpu-execution-evidence-005.tar.gz, SHA256
569bc2751afb82ac8eb4c32b25ddf82ce1cebe6deafad636e787353411d78780.
Matching extracted directory: /home/dan/projects/native-gpu-execution-evidence-005.
The native archive remains /boot/home/native-gpu-execution-evidence-005.tar.gz.
Published text evidence is experimental/native-gpu/evidence/native-copy-005.

The implementation is NativeExecution.inc included into the retained owner by
prepare-execution.py. The private native_execution/v1 ABI accepts only a report
pointer; the unloadable bridge owns no resources or callbacks. One mapping lease
and operation pin protect all GART backing. Ten independently owned native pages
hold private source/destination data and PPGTT/scratch tables. The limited DMA
backend rechecks all enumerated VT-d units with translation disabled; it never
disables VT-d or treats an enabled IOMMU as identity mapped.

Actual debugging, preserved in the evidence:

- Execution001 stopped before reset/publication on a forcewake safety check.
- Execution002 issued LOAD, then rejected the assumed CSB cursor progression.
- Execution003 recorded PCI Capture::Unstable during forcewake acquisition.
  Native runtime revalidation now checks actual access/power anchors twice,
  preserving ID/revision/BAR/command/D0 and owner checks without requiring
  volatile PCI Status/PME bits to equal a historical snapshot. Initial full
  capability admission is retained. Legacy diagnostic v7 behavior is unchanged.
- Execution004 proved all1024copiedwords, nonce/complement and breadcrumb correct,
  but its strict status reader expected slots0/1 while the GPU wrote slots1/2.
- Execution005 explicitly aligns MMIO, HWSP and reader initial cursor0. It accepts
  the actual activation03ff800000008019 and idle record0000800003ff8018,
  verifies the full copy and releases the forcewake group normally. The old
  helpers keep cursor11 as their default for existing callers.

The complete64-entry native MOCS/L3 expansion was checked against the pinned
Linux v6.6 table after review caught and corrected an L3 entry mismatch. Native
compile/export-link, fixed copy image, PCI anchor regression and captured CSB
regression checks passed. No broader stress/edge-case suite was run.
