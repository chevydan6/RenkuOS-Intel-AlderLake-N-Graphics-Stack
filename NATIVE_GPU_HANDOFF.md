<!-- SPDX-License-Identifier: MIT -->
# Native Intel N100 GPU handoff — 2026-11-14

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

**GPU command execution and an accelerated sample remain unproven.** The native
preparation module, bridge and client now compile/export-link against the actual
SDK. One physical invocation constructed nine PPGTT/scratch pages and the GGTT
ring/context/status/result backing, then released the unpublished resources with
mapping/tables/image/cleanup all0. The host image smoke also passes. See
[native preparation evidence](https://github.com/chevydan6/haiku-intel-gpu/tree/codex/i915-native-adaptation/experimental/native-gpu/evidence/native-prepare-001).

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

Important limits: rootPhysical is a pinned CPU physical address, not proven GPU
DMA admission. The image uses a fixed N100 profile; hardware execution must obtain
fresh actual identity/stepping and ownership. Existing mapping/v1 is explicitly
memory-only and permits end_set only because it has no GPU publication. Do not
submit these borrowed objects and then reuse its ordinary cleanup. The execution
owner/ABI must retain the complete request until retirement or established reset.

## Next work, in order

Intel documentation inventory: [docs/INTEL_GPU_DOCUMENTATION.md](docs/INTEL_GPU_DOCUMENTATION.md).
Use the N100 research map, the applicable Intel Xe-LP manuals and the N100
paths in pinned open-source drivers. A standalone N100 GPU programming manual
is **not a prerequisite** and must not block native development. Check concrete
platform differences as they arise; do not restart broad documentation searches.

1. Native preparation is complete for this checkpoint: build/export-link and one
   bridge/client call passed. Continue with execution integration below; do not
   expand or repeatedly rerun preparation without a new implementation reason.
2. Connect the prepared request to a retained native execution owner. Integrate
   existing forcewake/reset, fresh device/DMA admission, PAT/MOCS/cache policy,
   engine initialization, translation invalidation, engine HWSP/CSB setup and
   ELSQ descriptor publication. Adapt the integration rather than importing the
   full Linux device/scheduler/memory-manager model.
3. Prove the nonce/complement and ordered completion with fresh hardware context
   status and inactivity evidence. Keep the request/power/mappings alive through
   completion. A timeout must not free pages the GPU could still access.
4. Wire a small real GPU copy/fill sample using the existing BLT/reference logic.
   Provide a runnable sample and actual GPU-produced output. A CPU-prepared ring,
   reset result, passing unit test or GPU-written nonce alone is not the completed
   GPU-accelerated sample goal.

Keep checks proportional: compile and one focused smoke per change, then a bounded
hardware trial when the necessary path is ready. Broaden only for an observed
failure or an unresolved requirement needed for actual execution.

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
  test files removed. Production graphics/owner components were not replaced.
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
the laptop under /boot/home/i915-mapping-002/source. The installed resident owner
binary SHA256 was0a6ff95a029f125f89fecaab2b899c1346555793b2e620c922837eb27b5a483a
when inspected. An older gpu-memory-089-001 build has a different hash: do not
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

The preparation module bus_managers/intel_native_prepare/v1 remains resident
until reboot because of B_KEEP_LOADED, even though its installed test file and
bridge were removed. Its prepared backing was released successfully. Replacing
its file would not replace that loaded code; use a controlled reboot before
expecting a modified v1 implementation to run. No GPU commands were published.
Raw latest evidence is /boot/home/native-gpu-prepare-001 and locally
/home/dan/projects/native-gpu-prepare-evidence-001. Source bundle preparation is
at /home/dan/projects/native-gpu-prepare-bundle-001. Native logs were visible in
Terminal via NanoKVM; all normal cleanup and baseline checks passed.


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
released after the build. The next implementation task remains connecting the
owned request and this helper inside the retained owner, with actual device/DMA
admission and completion/retirement; then run the nonce proof and copy sample.
