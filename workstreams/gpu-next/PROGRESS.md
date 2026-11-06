# GPU next: memory ownership checkpoint

The verified HDMI desktop baseline remains `gpu-hdmi-cleanup-metal-057` (`eeb334df`). Offline work uses `/home/dan/renkuos-development/source/gpu-next-worktree`, branch `gpu/n100-memory-next`. No laptop, VM, NanoKVM, boot/media, host configuration or shared build resources were accessed.

## Patch 0065

Source commit `7cba9585`: `intel_extreme: roll back undelivered graphics allocations`.
Export: `gpu-driver/patches/0065-intel_extreme-roll-back-undelivered-graphics-allocations.patch`.

Production `device.cpp` now rejects undersized allocation/free requests before copying user memory. If allocation succeeds but delivering its descriptor fails, normal `intel_free_memory()`/GART teardown releases it. Failed teardown propagates its error and preserves GART's tracked backing; the driver never frees pages independently of the mapping lifecycle. If teardown fails, the caller has no delivered retry handle: backing remains tracked by GART but this patch does not solve orphan recovery or complete per-client ownership. No new ioctl, ABI version, mapping or GPU command was introduced.

Validation:

- 116 production-case host checks pass with AddressSanitizer and UndefinedBehaviorSanitizer: every short length, input faults, bad magic, allocation failure, successful delivery/free, output failure, teardown failure and 64 consecutive failed deliveries.
- The same tests against `eeb334df` fail 106 assertions, including leaked allocation after failed delivery.
- Five existing shared-layout ioctl cases pass.
- Source diff whitespace check passes.

Commands:

```sh
python3 gpu-driver/workstreams/gpu-next/memory-ioctl-host.py source/gpu-next-worktree/src/add-ons/kernel/drivers/graphics/intel_extreme/device.cpp
python3 gpu-driver/tests/shared-layout-ioctl-host.py source/gpu-next-worktree/src/add-ons/kernel/drivers/graphics/intel_extreme/device.cpp
```

The test generator supports `--emit-cpp PATH` to emit the exact standalone fixture for the native compiler without executing Python-side compilation or tests.

Native validation remains pending. Parent can freeze this worktree at `7cba9585` and build `intel_extreme.driver` using the existing coordinated native pipeline. No accelerant/GART/ABI rebuild is required by this patch alone, although building the normal complete component bundle remains useful integration coverage. A hardware smoke test should retain working desktop/SSH/HDMI; synthetic user-copy faults can first be exercised in an isolated VM, without probing new GPU command execution.

## Initial architecture findings

Current native Alder Lake support is display-only: accelerant engine count is zero, legacy ring allocation is skipped, and no modern engine submission path exists. Working scanout does not establish GPU rendering or 2D acceleration.

The native `intel_gtt.h` helpers validate 64-bit GGTT range, alignment, reserved aperture boundaries and the initial ADL-P/N 39-bit DMA limit. Firmware/stolen scanout mappings are preserved. System-memory binding is mediated by GART and its tracked partial-binding rollback. The shared framebuffer, raw aperture addresses and trusted accelerant MMIO are not an application-facing managed-buffer interface.

Important distinction: Linux `intel_ggtt.c`, pinned commit `f66469d9905b39d2c260a15d7d6db450f4db67c1`, lines 201–229 maps ICL+ GGTT uncached and deliberately omits the legacy `GFX_FLSH_CNTL` write in `gen8_ggtt_invalidate()` for that mapping. This does **not** justify adding guessed invalidation/forcewake writes to RenkuOS. Existing `wbinvd` cost and lifetime ordering require a separate review before optimization.

Primary reference: [Linux Intel GGTT implementation](https://github.com/torvalds/linux/blob/f66469d9905b39d2c260a15d7d6db450f4db67c1/drivers/gpu/drm/i915/gt/intel_ggtt.c).

Remaining prerequisites before executing modern GPU workloads:

- Kernel-owned buffer objects with non-address handles, ownership, bounds, pinning and completion references; current allocation/free addresses do not provide those properties.
- A supported GT power/wake lifecycle and engine discovery distinct from display power.
- A proven execution/context mechanism, interrupt/completion tracking, bounded waits and hang recovery.
- Validated command/surface addresses and formats; inherited legacy BLT opcodes cannot be assumed valid for Gen12.
- Measured hardware execution and CPU/software baselines before any acceleration claim.

Next audit is checking loss of allocation bounds at the accelerant's `size_t` to 32-bit ioctl boundary and defining the smallest managed-buffer ownership interface without enabling execution.

## Second checkpoint: exact allocation size

Commit `1fee8aaa` rejects zero and `size_t` requests greater than `UINT32_MAX` in accelerant `memory.cpp` before assigning the 32-bit ioctl field. It preserves the caller's output base on rejection. This prevents large requests aliasing a smaller backing allocation.

`memory-size-host.py` exercises the actual function with 111 sanitized boundary/failure cases, including 100 oversized values with nonzero low bits. The baseline fails 104 assertions. `--emit-cpp PATH` emits the standalone native fixture. Platform status macros are preserved when present, avoiding conflicts with Haiku `errno.h`; host compilation also passes when preexisting Haiku-style status macros are supplied.

Parent reported native driver compilation succeeded; complete native fixture validation remains with the parent. No hardware acceleration or managed-buffer support is claimed.

## Third checkpoint: preserve retained Intel aperture state

Commit `50691f19` adds a private Intel backend singleton-ownership flag. If GART unmap fails, the old registry object, allocations, mappings and module reference survive. Previously a normal reopen could overwrite that retained singleton and initialize its GGTT again. A second creation now returns `B_BUSY` before changing any state, mapping or PTE. Ownership is published only after complete initialization and cleared after deletion; failed initial map/PTE initialization remains retryable. The shared ABI, legacy graphics callback cookie and managed API are unchanged.

Validation:

- Ten tests execute actual Intel create/delete function bodies, with only hardware/mapping callbacks and the privileged cache instruction mocked. They cover initial failure, partial PTE setup failure, rejection without side effects, later cleanup/recreation, unrelated PCI devices and repeated deletion. Sanitizers pass; the old source fails seven assertions.
- A separate test executes actual generic GART map/unmap bodies against a simulated singleton backend. It verifies retained-ID retry, module references and global/per-aperture lock coverage; its guarded-backend mode rejects reopen and permits recreation after cleanup. Unguarded mode reproduces the old duplicate-backend creation hazard.
- Native compilation is pending for `intel_gart`; the parent coordinates it. Recovery/adoption of a retained aperture ID remains separate work. No modern GPU commands or hardware rendering are enabled.

```sh
python3 gpu-driver/workstreams/gpu-next/intel-singleton-aperture-host.py source/gpu-next-worktree/src/add-ons/kernel/busses/agp_gart/intel_gart.cpp
python3 gpu-driver/workstreams/gpu-next/gart-retained-aperture-host.py source/gpu-next-worktree/src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp --guarded-backend
```

Both generators support `--emit-cpp PATH` for native compiler validation. The manager test models locking assertions and references; it is not a concurrent kernel stress test or actual module-unload test.

## Fourth checkpoint: manager metadata allocation failure

Commit `f01e0b78` checks the nothrow `Aperture` allocation in generic GART `map_aperture()`. If it fails after backend creation, the manager deletes the new backend while still holding its module reference, drops that reference, and returns `B_NO_MEMORY` with no registry object. This is six production lines; no callback, ownership-recovery API or submission change.

The actual-manager fixture now injects class-specific metadata allocation failure and metadata initialization failure, with strict backend-delete/module-reference ordering checks. Its guarded-backend mode passes 13 cases under ASAN/UBSAN with exceptions and RTTI disabled. Running it against the prior manager reproduces a sanitizer-detected null-pointer member access on metadata OOM. Native compilation is pending for `agp_gart`; the parent owns that build window.

## Parent native validation: build 062

All four checkpoints above now compile in the matching RenkuOS x86_64 build
VM. The native fixtures pass 116 allocation-ioctl cases, 111 allocation-size
cases, 10 Intel singleton cases, and 13 generic GART lifecycle/OOM cases.
The host finalizer verifies the source manifest, linked module formats and
expected dependencies for intel_gart, agp_gart, intel_extreme and its accelerant.
This supersedes the pending-native notes above. These memory changes have not
been installed on the physical laptop, and no command submission or rendering
acceleration is enabled. The physical laptop remains on GPU cleanup build 057.
