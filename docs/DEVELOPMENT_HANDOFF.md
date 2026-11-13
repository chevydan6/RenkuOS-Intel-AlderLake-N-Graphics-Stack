# RenkuOS Intel N100 GPU: development handoff

**Repository:** `chevydan6/haiku-intel-gpu`  
**Evidence snapshot:** 13 November 2026  
**Audience:** the next AI or human developer continuing this project  
**Purpose:** locate and reuse existing implementation, distinguish verified behavior from plans, and resume development without repeating completed investigations.

This is a subsystem-level implementation and evidence inventory, not a certification that every line is correct or every historical test was rerun. It covers the native patch stack, execution helpers, LinuxKPI experiment, newer owned-GGTT mapping work, validation infrastructure, and historical/reporting branches. The source, exact revisions, and captured test evidence remain authoritative. Paths not explicitly marked experimental are relative to the native repository snapshot. Source references are collected in section 14.

## 1. Read this first

**The project has working physical display output and substantial memory, power/reset, context-construction, and compatibility-layer work. It has not demonstrated a GPU-executed workload, accelerated rendering, or hardware OpenGL/Vulkan.** Successful CPU tests on the N100 are not GPU execution. [N1, N2, E1, E2]

The most important update since the repository cleanup is on `experiment/i915-linuxkpi`: an owned native GGTT mapping sandbox passed **eight physical invocations of 38,927 checks each**. Its kernel-only CPU aperture alias corrected a real SMAP fault. This is usable development groundwork, not an absent component and not a completed submission engine. [E3, E4]

The two relevant source recipes differ:

- **Default native stack:** pinned RenkuOS source plus all 118 numbered patches, through 0120 with 0098/0099 absent.
- **Physical mapping sandbox:** the same RenkuOS source pin plus 0001-0096, then two mapping-specific patches outside the numbered series. It does not include the later ACPI/module stack. Do not silently mix these recipes. [N3, N4, E3]

For the next implementation, inspect the existing mapping sandbox, owner, context helpers, nonce encoder, and completion design before creating replacements. The immediate goal is one real BCS0 nonce/complement write with completion and retirement evidence, not broad Linux compatibility or a new renderer. [N5, E2, E3]

### First-session procedure

1. Fetch the repository refs and record the exact branch, commit, and worktree state. Compare them with section 2; this document is a dated snapshot.
2. Read sections 3, 6, and 11. They describe what is actually built, the newer mapping work, and the next integration boundary.
3. Inspect only the implementation and evidence relevant to the selected task. Do not reread every chronological log or rerun the entire historical investigation.
4. Reproduce the affected host tests and identify the native source recipe before making changes. Hardware access and installed artifacts require a fresh check; an old log is not a live connection.
5. Finish a small code/test/evidence change, then update this handoff's affected entries. A plan, agent cycle, or compilation-only result must not be recorded as physical GPU execution.

## 2. Exact baselines and branch roles

| Branch | Observed head | Role at this snapshot |
| --- | --- | --- |
| `gpu/renkuos-n100` | `49a32ff126331d86fefe12c0a875936b6926eb2e` | Default native baseline; PR #2 cleanup squash-merged here. |
| `experiment/i915-linuxkpi` | `3060587a5a28a7831049a858f933cdeadb25c06d` | Linux source reuse, native services, and the newer physical GGTT mapping sandbox. Not merged into default. |
| `gpu/native-memory` | `ac2c05acd2186a069b4c5a648c986de9a999a589` | Older native-memory development checkpoint. |
| `gpu/alderlake-n` | `138de017633383410a695bf444ad5b99e586d917` | Preserved historical Haiku/Alder Lake baseline. |
| `gpu/hdmi-pipe-fix` | `ce03973142c48fe8250dc7eb7136bf71c8306768` | Historical divergent display work; inspect unique changes before deleting or consolidating. |
| `rondoflow/n100-results-20261112` | `9f273a18fe4ccc90b9a57b6e578c774d32656751` | Workflow-cycle publication records, not automatic driver integration. |
| `cleanup/repository-reorganization` | `2b11f75faf0cec719245077b1b8948e8825eb6e4` | PR #2 source branch; its content was squash-merged as `49a32ff`. |

The LinuxKPI experiment started from native commit `30dfdb32957215b4c59da3ac763ba14ab0c482d0`. Do not interpret the later squash merge as containing its experimental additions. Branch names and relative commit counts may change; re-query refs instead of treating this table as permanent. [B1, N6, E1]

### Upstream and runtime identities

- Native RenkuOS source pin: `4799bb5a188c4bb6bbef967287640cc21a061a0d`.
- Preserved Haiku source pin: `4f1074d5e14f59e0f5e5c4938c8a50d1290fa59c`.
- LinuxKPI imports: Linux v6.6, `ffc253263a1375a65fa6c9f62a893e9767fbebfa`.
- Earlier native execution research also uses other pinned Linux snapshots. Use each reference manifest; do not assume every helper was derived from v6.6.
- Reported physical target: Intel N100; host bridge `8086:461c`; GPU `8086:46d1`, revision 00; RenkuOS `hrev60072+518`, matching development package, GCC 13.3.
- LinuxKPI VM checkpoint: RenkuOS build00081, source `af22bfb13d758e13a2d2f6ab204bdbc3e4aab42f`, matching `hrev60072_780` SDK and GCC 15.3. This differs from the older physical kernel. Do not interchange binaries merely because both are x86-64. [N1, N2, E1]

## 3. Capability and evidence ledger

The labels below describe the strongest cited evidence, not universal correctness. **Host** means Linux-side tests. **Native VM** means code running or compiling under RenkuOS in a VM. **Physical CPU** means real N100 kernel/CPU activity. **Physical MMIO** means actual device-register operations. **GPU execution** requires a submitted workload and independently checked device completion.

| Capability | What exists | Evidence boundary |
| --- | --- | --- |
| PCI discovery and GGTT basics | N100 identification; Gen8+ BAR layout; 64-bit PTE handling and bounds checks. | Host/native checks and physical discovery; not engine execution. |
| HDMI desktop | Native 1920 x 1080 RGB8 output, historically build 057. | Physical display; no rendering acceleration implied. |
| GART lifecycle | Allocation, rollback, retained ownership, cleanup retry, lock/hash/semaphore fixes. | Host controls, historical native builds and selected physical memory probes. |
| Retained memory owner | Shared owner module, display/request leases, operation pins, observation APIs. | Host/native and selected physical operations; no complete submission lifetime. |
| PPGTT/page backing helpers | Encoding, offline trees, wired-page provider and reserved-range checks. | Host/native helpers plus selected physical kernel allocation checks. |
| GT and BCS control | PCI/ACK observations, primary/group forcewake, fixed BCS inventory, reset-only path. | Physical MMIO observations and one successful bounded reset trial; earlier failure retained. |
| Context and completion helpers | Descriptor/status ledger, bounded CSB reader, 16 KiB initial context-image builder. | Host/native CPU tests; not wired to a proven live request. |
| Command encoding | Nonce/complement encoder, offline copy packets, original Linux ring and batch-start slices. | CPU construction tests only. |
| LinuxKPI CPU services | Original allocator/tree/refcount code plus scoped native locks, task/time/pages/completion resources. | VM and physical kernel-service suites: 16 x 62 checks each. |
| Owned GGTT mapping sandbox | Four GART objects, private request lease, kernel-only CPU alias, bounds and cleanup. | Physical CPU aperture path: 8 x 38,927 checks; not DMA admission or GPU execution. |
| ACPI/module/EFI extensions | Later native patches for descriptor admission, lifecycle custody and root validation. | Source and focused tests; no blanket claim that all were installed or natively validated together. |
| Real command submission, retirement, acceleration | Integration still unfinished. | No established GPU-executed workload in the inspected checkpoints. |

References: [N1-N5, N7-N9, E1-E5]. Do not add test counts from different revisions into one purported current pass total.

## 4. Native implementation inventory

### 4.1 Discovery, GART layout and legacy-path suppression

Numbered patches 0001-0012 establish foundational changes: the N100 bridge/device pair, correct GGTT BAR placement, 64-bit PTE stores and validation, stolen-memory decoding, firmware mapping preservation, generic GART unbind/allocation failure handling, and suppression of unsupported legacy ring/2D/overlay behavior. This is not a missing 32-bit-to-64-bit GGTT conversion waiting to be rediscovered. [N1, N3]

Primary generated-source locations are `src/add-ons/kernel/busses/agp_gart/intel_gart.cpp`, its `intel_gtt.h`, the generic `src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp`, and the Intel kernel-driver/accelerant directories. These files are reconstructed from upstream plus patches; this repository is not itself a complete RenkuOS source checkout. [N4]

### 4.2 Boot diagnostics and display implementation

The project has boot/framebuffer metadata diagnostics, bounded descriptor checks, physical inventory tools, and the Intel display work needed for the tested HDMI mode. The display sequence covers DDI/pipe discovery, EDID selection, Type-C/transcoder planning, Dekel PLL and signal helpers, plane programming, power prerequisites, clone ownership, RGB8/DVI-compatible output, and DDB/MBUS placement corrections. Patches 0013-0064 contain the principal display-development progression, not one independently usable patch per feature. [N1, N3, N10]

Navigation: `diagnostics/`, `physical/`, `experiments/dekel-address/`, display-focused files in `tests/`, and `results/2026-11-06-hdmi-integration.md`. The legacy display baseline identifier is `gpu-hdmi-cleanup-metal-057` / source checkpoint `eeb334df`. Preserve its evidence, but do not assume the current installed owner/backend still equals that old bundle. [N7, E4]

### 4.3 Generic allocation, failure and recovery work

Patches 0065-0079 address undelivered allocation rollback, 32-bit ioctl size truncation, singleton ownership, metadata allocation failure, cleanup reporting, retained-aperture retry, callback lifetime locking, rounding/placement bounds, retirement admission, nested lock deadlock, failed hash publication, semaphore cleanup, and teardown iteration/resource handling. [N3, N7]

The workstream includes production-body host harnesses rather than only stand-alone replacement algorithms. Important lessons: an earlier test substituted an unlocked helper and missed the real nested lock; hash insertion can fail; failed unbind must not free still-mapped backing; ordinary hash removal can invalidate a live iterator. `GPU_DRIVER_REVIEW.md` records the findings and later corrections. Do not cite its original defects as still unfixed without checking the corresponding later patch and regression. [N7, N8]

Useful harnesses in `workstreams/gpu-next/` include `memory-ioctl-host.py`, `memory-size-host.py`, `gart-recovery-host.py`, `gart-threaded-host.py`, `gart-publication-host.py`, `gart-size-host.py`, `gart-placement-host.py`, and teardown/lock-release tests. These complement, rather than replace, the exported bundle's GART tests. [N7, N8]

### 4.4 Retained owner and interrupt lifetime

Patches 0080-0085 add lease-state bookkeeping, persistent unresolved-allocation state, the retained `intel_memory_owner` module, failed-free retention, N100 display-memory routing, and aborted-initialization handling. Generated source includes `IntelMemoryOwner.cpp` and private owner/lease headers. Display leases, request leases and operation pins are different things; do not reuse a display lease as permission to execute a GPU workload. [N3, N7]

Patches 0086-0088 address early interrupt admission, MSI cleanup tracking and private interrupt ownership. Separate IRQ/vector/dispatcher and retrace research/tests exist in `workstreams/gpu-next/` and `research/n100-submission/`. Some were isolated kernel experiments or local-source fixes. Their existence is not proof that a complete interrupt-unload/engine-retirement protocol is integrated into the published driver. [N3, N7, N11]

### 4.5 Offline GPU address spaces and native page backing

Existing helper files under `workstreams/gpu-next/` include:

- `IntelN100Ppgtt.h`: checked PPGTT encoding, initially 39-bit DMA and 48-bit GPU virtual-address constraints.
- `IntelN100PpgttTree.h`: owned offline table construction and failure handling.
- `IntelN100KernelPages.h`: kernel wired-page provider and reserved-range policy.
- `IntelDmarInventory.h`, `IntelDmarReserved.h`, `IntelVtdObservation.h`: DMAR parsing, exclusions and bounded VT-d observation.

Recorded checkpoints include 2,150 encoding cases, 69 tree cases, three physical kernel cases covering 42 areas, a physical DMAR snapshot, two read-only VT-d observations, and 42 physical reserved-range checks. These are earlier evidence sets, not one fresh combined run. CPU physical pages, GPU virtual offsets and device DMA addresses are different domains; historical TES-clear samples do not establish a lasting identity-DMA contract. [N9, N11]

### 4.6 Power, forcewake, inventory and BCS reset

Patches 0089-0096 and the `IntelN100PciPower`, `IntelN100GtAck`, `IntelN100Forcewake`, `IntelN100BcsInventory`, `IntelN100ResetDomains`, `IntelN100ForcewakeGroup`, and `IntelN100BcsReset` helpers provide the existing device-control foundation. The owner is the coordinated register-access path; a second experimental mapper is not needed merely to repeat those capabilities. [N2, N3, N9]

Physical evidence includes one primary ACK handshake, fixed BCS register inventory, media-fuse observation, a four-domain handshake, and A34's bounded BCS reset with two reset rounds and released domains. A33 stopped after preparation writes with no reset rounds; its root cause was not established. A34 does not retrospectively explain A33 or prove repeated reset reliability. One-shot admission was consumed for the relevant boot. [N2]

### 4.7 Context, CSB and packet helpers

`IntelN100ContextStatus.h` implements checked descriptor/status bookkeeping; `IntelN100CsbReader.h` implements bounded completion-status-buffer reading; `IntelN100ContextImage.h` constructs the initial image; `IntelN100NonceStores.h` provides the minimal nonce-store encoding. Their software evidence includes 2,357 descriptor/status cases, 135 CSB cases, 715 initial-image cases, and 32 nonce-encoding cases. Context-image comparisons use pinned extracted Linux routines. A saved STOP_RING mask-bit mismatch was caught and fixed before deployment. [N2, N9]

`tests/linear-copy-test.cpp` separately exercises experimental linear-copy packets, with 60,070 offline checks in the host regression. That packet experiment has no established hardware submission path. A modulo CSB cursor, a matching nonce, or a successful image comparison alone does not prove a request is retired. [N2, N5, N12]

### 4.8 Later ACPI, module and EFI work

The complete native series also contains 0097 and 0100-0120. Their main areas are DMAR preflight; DPC ownership/admission; module quarantine, dependency custody and transition pins; unlocked module initialization/uninitialization; ACPI callback/interrupt/table admission; a DMAR descriptor interface; and EFI ACPI root pointer, extent, kind, alignment and length fixes. The missing numbers 0098/0099 are numbering gaps, not evidence that two files should be invented. [N3]

Focused host suites and design records exist under `tests/acpi-*-host/`, `tests/efi-acpi-host/`, `tests/dpc-ownership-host/`, and `research/n100-submission/`. The standard source preparation applies the whole current series, but the existing GPU CI selection is not equivalent to executing every specialized ACPI/module test or booting a full kernel containing those changes. [N4, N12]

A particularly important boundary: `DMAR-PRODUCER-PREREQUISITE.md` is explicitly a **design-only** analysis. It says the inspected descriptor path does not provide its proposed coherent-source guarantee; the current preflight does not configure reserved backing or establish DMA admission. Do not report the proposed trusted snapshot producer as built. Also do not convert that historical proposal into an endless requirement to prove every possible firmware writer absent. For the selected next implementation, choose and document a concrete boot/runtime trust model, its evidence and residual assumptions. The physically tested mapping sandbox deliberately uses the pre-0097 baseline. [N13, E3]

## 5. LinuxKPI and original i915 reuse inventory

This implementation lives on the experimental branch under `experimental/i915/`, not under the native branch's similarly named directory. Its `upstream/`, `include/`, `src/`, extraction/build scripts, tests and evidence are separate from the global patch stack. [E1, E2, E6]

### Completed reusable pieces

**Original allocator and trees.** Complete Linux `drm_mm` and underlying tree code were imported and tested, including high virtual ranges, fragmentation, alignment, overlap and exhaustion. A 20,000-transition host stress test is recorded. This allocates virtual ranges; it does not by itself allocate DMA pages or install GPU page tables. [E1, E6]

**Original reference counting.** Linux `refcount`/`kref` algorithms retain saturation, ordering and final-release behavior. Host tests include 400,000 get/put pairs. Linux-required arithmetic behavior is retained with `-fno-strict-overflow`; the recorded sanitizer failure without it was not solved by disabling the sanitizer. [E1, E6]

**Scoped native services.** Implemented adapters include reached atomics, semaphore-backed mutexes, spinlock/IRQ exclusion, registered task identity, time access, native wired pages, native completion/deadline resources, and actual diagnostic driver/module glue. Some Linux concepts remain only declarations or scoped substitutes: native completion resources are not the entire Linux waitqueue/fence API; task registration is not a full scheduler/task-lifetime service; RCU pointer operations are not grace periods. [E1, E2]

**Wound/wait prerequisites.** Original acquire-context code, stamps and scoped identity tests exist, including a 40,000-context host test. Full `ww_mutex_lock` semantics were not implemented merely by making headers compile. [E1, E2]

**Original command-construction slices.** The original bodies of `intel_ring_update_space`, `intel_ring_reset`, `intel_ring_begin`, `gen8_emit_bb_start_noarb`, and `gen8_emit_bb_start` are reused through an explicitly adapted slice. Packet tests exercise wrapping, guards and addresses above 4 GiB. The private request/ring layouts are not full Linux i915 ABI layouts. [E1, E2, E6]

### Native and physical service results

The native diagnostic was built, loaded, exercised and unloaded in a RenkuOS VM, then on the physical N100 with its older matching SDK. Each suite records 16 invocations x 62 checks, including real kernel workers and cleanup. The separate execution-preparation object still has unresolved request timeline/ring-space-wait imports; the installed CPU-service diagnostic does not become a GPU driver merely because those services pass. [E1, E2]

### Open compatibility boundaries

The latest cited whole-file `intel_ring.c` compilation boundary includes DRM VMA/rwlock interfaces, completion/IDR/radix-tree dependencies, full wound/wait locking, fences/RCU/VM semantics and real request retirement. The bounded execution slices still require `i915_request_timeline` and `renku_ring_wait_for_space`. These are real unimplemented interfaces, not permission to substitute always-successful stubs. Full-file compiler closure is a useful inventory, but is not the primary workload goal unless a selected execution path genuinely requires it. [E2]

## 6. Newest stepping stone: owned GGTT mapping

### Exact source and interface

Read `experimental/i915/mapping/README.md` and `owner-mapping.patch` at experimental commit `3060587`. The recipe is upstream RenkuOS `4799bb5...`, patches 0001-0096, `gart-invalidation.patch`, then `owner-mapping.patch`. The global 0097+ stack is not part of this physical candidate. [E3]

The private module name is `bus_managers/intel_memory_owner/mapping/v1`. It exports `begin_set(intel_memory_mapping_set*)` and `end_set(uint64 token)`. One private request lease and operation pin own four buffers. Each descriptor provides a kernel CPU aperture alias, a GGTT offset and a size. It does not export a device DMA address. [E5]

The four fixed backing roles are ring 4 KiB, batch 4 KiB, result 4 KiB, and context 64 KiB: 19 pages total. **These labels do not mean a valid ring or hardware context was initialized.** The 64 KiB sandbox reservation must not be confused with the independently tested 16 KiB initial context-image helper. An engine status/HWSP allocation and the final execution layout still need explicit integration. [E3, E5, N2]

### Ownership and mapping behavior already implemented

GART remains responsible for placement, physical backing, PTE writes, invalidation and unbinding. The sandbox shares the retained owner instead of creating a second aperture owner. It checks reserved bounds and overlaps before publishing descriptors. Kernel-only clones preserve the aperture's device-cache type; original GART addresses are retained separately for freeing. [E3, E5]

A failed begin does not publish a partially valid set. An uncertain set is retained. A failed end records which individual frees succeeded, retains the remaining state, and allows cleanup retry without double freeing. That cleanup is valid for this **memory-only** interface because no GPU work can refer to its buffers. Execution requires a new, real device-use/retirement lifetime; do not merely expose submission through the existing interface and keep the same end semantics. [E3, E5]

The GART candidate adds an N100 UC GGTT invalidation register operation while retaining native cache writeback. The physical probe verifies CPU aperture access, not every engine/GuC TLB or GPU cache policy. Avoid choosing between older invalidation research and this later implementation by prose alone: compare the exact source reference, cache mapping and tested behavior. [E3, N7]

### Physical results and preserved failures

Eight invocations recorded 38,927 checks each: 311,416 checks, 32 object allocations/releases and 152 page equivalents. Two CPU patterns were written and read across all buffers. The evidence explicitly says this is not GPU command execution or DMA-readiness proof. The final four-run wrapper completed with exit 0 and current load/unload/device-removal checks. [E4]

Three failures are important for future agents:

1. **SMAP fault:** direct kernel stores to the old user-permission aperture faulted. The correction was a kernel-only clone, not disabling SMAP or changing the expected test result.
2. **Cleanup timing:** an earlier wrapper returned failure after successful mapping tests because devfs removal was asynchronous and the unload match included historical messages. The fixed runner waits for this invocation's removal/unload evidence; the earlier failure remains recorded.
3. **NanoKVM recovery side effect:** HID reset unexpectedly selected default media. The custom external test image and attachment flags were restored. Do not assume a keyboard-reset operation leaves storage configuration untouched. [E4]

### Reported installed state, not a live inspection

The final mapping owner and Intel GART candidates were left installed on the external RenkuTest volume. The diagnostic driver/device was removed; the native display driver, accelerant and kernel were not replaced. Original owner/backend binaries remain under `/boot/home/i915-mapping-001/recovery/`; the later target tree is `/boot/home/i915-mapping-002/`. A rollback script was checked but not executed. [E4]

These are recorded locations, not proof that the target is currently reachable or unchanged. Before deployment, inspect its current kernel/SDK, boot volume, binary hashes and hardware lease. Do not install a complete 118-patch bundle over this state just because default-branch CI passes.

## 7. Evidence, tests and infrastructure already built

### CI and source reconstruction

`tools/prepare.py` exports the pinned upstream subset, applies numbered patches in sorted order with `git apply --check`, preserves original controls, extracts testable production methods, copies the test/tool payload, and writes SHA-256 manifests. The output directory must be new. `tools/check-source.py` verifies source expectations, original controls and payload identity. [N4]

`.github/workflows/source-check.yml` now runs the existing GPU host suite on the exact PR base and candidate revisions, retaining sanitizer and original-code controls. Both sides passed for cleanup candidate `2b11f75` versus base `30dfdb3`. After squash merge, push run **43405169464** also passed on `49a32ff`. This is actual patch reconstruction and compiled Linux-host regression coverage, not native driver compilation. [N6, N12, N14]

The suite includes DDI selection, GGTT ranges/PTE/stolen decoding, owner/forcewake/reset helpers, context status/CSB/image, framebuffer descriptor bounds, GART faults, kernel-resource controls and offline linear-copy packets. Existing original-code controls intentionally fail in known ways and are counted as successful regression detection, not silently ignored failures. [N12]

### Native component and VM tooling

The component build infrastructure includes `tools/Makefile.haiku`, `tools/run-native.sh`, `tools/run-native.py`, `tools/finalize-native.py`, `tools/validate-artifacts.py`, `tools/regression-control.py`, and VM orchestration tools documented in `docs/BUILD.md`. It validates ELF architecture/entry points, dependencies, artifacts and source manifests. It is not a full official Jam OS-image build. Older native passes do not automatically validate later patch additions. [N15]

The LinuxKPI experiment additionally has its own `target-build.py`, upstream verification/extraction scripts, Makefile targets, native-service VM bundle/runner, and mapping-specific bundle/build/probe scripts. These are separate recipes; select the one matching the intended evidence. [E1, E3, E6]

### Evidence organization

`tests/evidence/` contains recorded native workstream results. Relevant families include physical memory/copyout/page/DMAR/VT-d probes, PCI/ACK/forcewake, BCS inventory/reset, and context/CSB/image host/native records. The experimental branch adds `experimental/i915/evidence/native-vm-006/`, `physical-n100-001/`, `mapping-host-001/`, and `physical-mapping-001/`. [N2, N9, E1, E4]

`results/` supplies dated summaries; `research/n100-submission/` holds design/provenance; `research/terra-n100/` holds independent checks and source snapshots. `research/local-model-evaluation/` is failed-agent/game-port evaluation evidence, not a working GPU or Luanti port. Raw images, private connection material, large binary archives and some full logs are deliberately outside Git. A repository-relative historical reference can therefore identify a local artifact, not a downloadable tracked file. [N2, N8, N16]

### Repository organization and audit tooling

PR #2 moved nine guides under `docs/`, preserved original-path pointers, added navigation/patch inventories and merged as `49a32ff`. `tools/audit-repository.py` checks patch targets and inline local Markdown destinations. Its one known broken archive link is keyed to exact path, destination and original Git blob; altered bytes or a new broken link fail its controls. [N6]

`tools/map-dependencies.py` is a heuristic explicit-path mapper, not a semantic C++ dependency graph. It scans filesystem contents, so generated/untracked outputs can affect counts; do not equate its count with `git ls-files`. The audit's expected patch count is currently 118: when deliberately adding a patch, update the relevant inventory checks rather than treating the fixed count as permanent architecture. [N17]

## 8. Reproduction entry points

The following commands come from existing recipes. They are not claims of a new run performed while writing this handoff. Use fresh output directories and preserve failing logs. The host commands do not deploy drivers.

### Native default stack: host preparation

Run at the root of a checkout of the pinned native snapshot or its reviewed successor:

```sh
python3 tools/audit-repository.py --strict --output /tmp/n100-audit.json
python3 tools/prepare.py --output build/handoff-native
python3 tools/check-source.py build/handoff-native
python3 tests/owner-observation-host.py \
  --source build/handoff-native/source --output build/handoff-owner
python3 tools/run-gart-range-host.py build/handoff-native
python3 tools/run-gart-lifecycle-host.py build/handoff-native
python3 tools/run-gart-allocation-host.py build/handoff-native
```

Use `.github/workflows/source-check.yml` for the complete tested host command list. In a matching RenkuOS guest, the prepared bundle's documented `tools/run-native.sh` builds/tests components; host finalization is a separate step. Read `docs/BUILD.md` for the correct working directory, matching SDK, collection and finalization procedure. [N12, N15]

### LinuxKPI components: experimental branch

After selecting the experimental snapshot, run within `experimental/i915/`:

```sh
python3 verify-upstream.py
make host-object
make allocator-test
make refcount-test
make ww-context-test
make emitter-test
```

Target-header builds use `make target-objects RENKU_SOURCE=/path/to/pinned/source`, with the matching source headers and documented compiler setup. `SANITIZER_LDFLAGS` is needed only where the compiler's matching runtimes were extracted locally. Do not disable sanitizer checks to make a configuration mismatch disappear. Whole-file attempts are dependency probes and can intentionally return compilation failure. [E1, E6]

### Mapping sandbox: separate source recipe

Run at the experimental repository root:

```sh
python3 experimental/i915/mapping/prepare-bundle.py \
  --source /path/to/pinned/RenkuOS-source --output /new/mapping-bundle
python3 experimental/i915/mapping/run-host-tests.py \
  --source /new/mapping-bundle/source --output /new/mapping-host-results
```

In the prepared target bundle, `bash -x mapping/build-native.sh` compiles without installing. The separate native test runner installs its diagnostic and exercises the real mapping candidate; that is a hardware operation, not another harmless host unit test. Use the current mapping README and verified installed-state procedure rather than replaying an old staging script. [E3]

## 9. Where to find the work

| Repository area | What to use it for |
| --- | --- |
| `patches/` | Native changes applied to pinned upstream source; use `docs/PATCH_INVENTORY.md` for all 118 names. |
| `workstreams/gpu-next/` | Reusable execution/memory helpers, production-body harnesses and historical progress. |
| `tests/` | Host/native fixtures and protocol/bridge clients; read individual runners for environment requirements. |
| `tests/evidence/` | Captured results and provenance, not active implementation. |
| `diagnostics/`, `physical/` | Inventory and physical diagnostic implementations/procedures. |
| `experimental/`, `experiments/` on native | Offline packet/PLL experiments; not the full LinuxKPI tree. |
| `experimental/i915/` on experimental | Original Linux imports, adapters, extraction/build/test tools and native diagnostic. |
| `experimental/i915/mapping/` | New physical GGTT-backed mapping candidate and exact recipe. |
| `tools/`, `installation/` | Source preparation, native build/finalization, deployment/VM utilities and associated guidance. |
| `research/`, `results/`, `coordination/` | Designs, source references, independent reviews and historical milestones. |
| `docs/`, root pointer files, `drivers/README.md` | Navigation and current cross-branch guidance; `drivers/` is not a relocated driver source tree. |
| RondoFlow results branch | Agent-cycle reports and publication mechanics, not proof of integrated driver features. |

The root LICENSE and per-file upstream notices still govern their respective files. Retain original import notices/provenance; do not relabel all experimental upstream code as MIT because a local adapter uses MIT. [N1, E1]

## 10. Stale statements and mistakes not to repeat

**A29 still blocked:** the task board retains an earlier pending state, but the later reset/context report records native/physical inventory and A34. Read the later evidence. [N2, N11]

**No owned GGTT buffers:** outdated for the experimental mapping snapshot. Four owned buffers and kernel CPU aperture access have physical evidence; execution-ready device lifetime is the missing step. [E3, E4]

**Build 057 or A34 is the complete installed system:** historical display/reset descriptions do not supersede the newer mapping-owner/GART installation record. Recheck actual binary identities. [N2, E4]

**All numbered patches were installed together:** CI applies 118 patches, while the physical mapping sandbox uses 96 plus two different patches. These are different configurations. [N4, E3]

**Native compilation was never available:** older LinuxKPI checkpoint paragraphs say that; later VM and physical service sections supersede them. The partial execution object's unresolved imports remain real. [E1, E2]

**A declaration is an implemented service:** `__iomem`, RCU metadata, Linux VM declarations, ww context initialization and native semaphore completion resources do not implement full MMIO ordering, grace periods, Linux VM, ww locking or fences. [E2]

**CPU aperture readback proves GPU DMA:** it proves the tested CPU mapping path, not IOMMU policy, engine translation/cache behavior or device execution. [E3, E4]

**Matching a nonce is enough:** the proof must also exclude CPU fallback and establish completion/retirement. The negative control must use the same verifier with submission disabled. [N5]

**A successful CI job is a physical driver result:** the current source workflow is a Linux-host suite. It is useful evidence, but neither native kernel load nor hardware submission. [N12, N14]

## 11. Next development step: reuse, compose, execute

This is a recommended continuation, not a claim of implemented functionality. The earlier cleanup decision to preserve the native foundation still makes sense, but it must account for newer experimental code. The experimental branch now contains a native-owner mapping implementation; "native versus LinuxKPI" is no longer a useful all-or-nothing choice. Reuse that concrete work selectively instead of rebuilding its allocator/alias logic. [N18, E3]

### First deliverable: one integrated execution-buffer set

Take the already tested mapping candidate and define the missing execution lifetime explicitly. Reconcile its 0001-0096 baseline with the intended integration branch; decide whether the later descriptor-only ACPI stack is necessary for this concrete request. Preserve a precise patch/source ledger, not an assumption based on the default branch name.

Add or compose the actual required backing for context, ring, engine HWSP/status, result and page tables. Use the existing context image and nonce helpers. Keep CPU alias, original free address, GGTT offset and any independently established DMA address distinct. The immediate acceptance is real owner/backend integration with production-body tests and a matching native component build, not another stand-alone mock that pretends mappings exist.

### Then close the concrete publication dependencies

Document and implement the chosen DMA/IOMMU trust model, CPU/GPU visibility and necessary cache/TLB policy; initialize the actual BCS context/engine using the existing control work; connect descriptor publication, completion polling, context/status evidence and reset-generation handling. Reuse existing forcewake/reset helpers where their contracts match. Investigate only dependencies reached by this selected path.

The first target remains a kernel-constructed direct-ring nonce/complement store. A separate batch allocation in the mapping sandbox is not a reason to expand the accepted minimal command into general userspace batch submission. Choose one request and one engine first. [N5, E3]

### Definition of the first GPU execution result

Record exact source/binary/target identity, fresh nonce and poison, submitted bytes and buffer identities, bounded completion result, context/engine state and cleanup/retention outcome. A disabled-submission run must fail the same verifier. The successful path must not write the expected result from the CPU. Completion alone must not cause backing to be freed while the device can still use it. These are direct correctness conditions for the experiment, not a mandate to finish a general scheduler, full Linux DRM, Vulkan or every hypothetical recovery feature first. [N5]

After that, use the result to prioritize repeats, recovery, useful copies/rendering and userspace integration. Do not claim those later features from the first nonce store.

## 12. Working agreement for future AI sessions

Dan's latest requested workflow is implementation-led and token-efficient: Sol handles implementation; Terra performs targeted research and independent verification; Astra handles significant architecture review or a genuine stall. Use a deterministic watchdog for routine process monitoring rather than continuous model discussion. Older files assigning every task to Astra are historical work allocation, not a permanent requirement.

Maintain one active milestone and a concrete next action. When blocked, state the exact missing function, failing test, source mismatch or unavailable environment. Do not turn speculative risks into an expanding prerequisite list. Do not silently fabricate success, erase failed evidence, or replace tests with assertions that only validate a mock's own assumptions.

Use separate worktrees and existing resource coordination for shared build/VM/physical operations. A restarted agent must inspect whether a hardware operation already ran before repeating it. Do not claim access to the user's laptop merely because a historical path or MCP connection is described here. External upstream submissions remain subject to the owner's approval.

These are project working preferences carried forward from the owner's conversations. They are not independent proof of code capability and do not override a newly stated owner decision.

## 13. Keep this handoff useful, not another stale report

The repository is enough **only when it contains a maintained entry point like this**. A separate chat transcript or giant repeated status document is not a substitute for source and tests. Keep the Markdown file in Git; treat any Word/exported copy as a dated reading copy.

Update the relevant capability row and next-action paragraph in the same PR that changes behavior. Include the code commit, source recipe, test environment, command/result location, and remaining gap. Use the strongest evidence label actually supported. If a result supersedes an old statement, identify it explicitly instead of appending another contradictory paragraph.

Suggested compact milestone record:

```text
Milestone:
Implementation branch and commit:
Upstream/source recipe:
Files or public/private interfaces changed:
Evidence class: HOST / NATIVE-VM / PHYSICAL-CPU / PHYSICAL-MMIO / GPU-EXECUTION
Reproduction command and result path:
Observed result and preserved failures:
Installed-state change, if any:
Remaining concrete gap:
Next action:
```

At each new session, compare the recorded pins with current branch heads. Read only relevant diffs and evidence if nothing material changed. Do not rewrite this entire document after every small test, and do not let updating it replace implementation.

## 14. Pinned source register

All repository links below are private-account links. Access requires permission. The document describes recorded evidence; no new physical test was run to author it.

### Native baseline and shared work

- **B1:** [Observed branch collection](https://github.com/chevydan6/haiku-intel-gpu/branches). Mutable discovery link; exact observed heads are recorded in section 2.
- **N1:** [Native README](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/README.md).
- **N2:** [A29-A37 reset/context publication checkpoint](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/results/2026-11-07-reset-context-progress.md).
- **N3:** [All numbered native patches](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/docs/PATCH_INVENTORY.md).
- **N4:** [Pinned source preparation implementation](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/tools/prepare.py).
- **N5:** [First GPU execution design and verification protocol](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/GPU_FIRST_EXECUTION_PLAN.md).
- **N6:** [Merged cleanup PR #2](https://github.com/chevydan6/haiku-intel-gpu/pull/2); [squash commit](https://github.com/chevydan6/haiku-intel-gpu/commit/49a32ff126331d86fefe12c0a875936b6926eb2e).
- **N7:** [Memory-owner workstream history](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/workstreams/gpu-next/PROGRESS.md).
- **N8:** [Independent memory-management review](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/GPU_DRIVER_REVIEW.md).
- **N9:** [Early physical memory, PPGTT and forcewake evidence map](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/results/2026-11-07-gpu-progress.md). Later reset/context results supersede its pending A29 status.
- **N10:** [HDMI integration report](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/results/2026-11-06-hdmi-integration.md).
- **N11:** [Historical task board](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/coordination/TASK_BOARD.md); [hardware record](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/coordination/HARDWARE_TEST_RESULTS.md).
- **N12:** [Executable GPU host-regression workflow](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/.github/workflows/source-check.yml); [completed base/candidate comparison](https://github.com/chevydan6/haiku-intel-gpu/actions/runs/43402836153).
- **N13:** [DMAR producer prerequisite: explicitly design-only](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/research/n100-submission/DMAR-PRODUCER-PREREQUISITE.md).
- **N14:** [Successful post-merge host regression on 49a32ff](https://github.com/chevydan6/haiku-intel-gpu/actions/runs/43405169464).
- **N15:** [Native component build and finalization guide](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/docs/BUILD.md).
- **N16:** [RondoFlow publication-lane description](https://github.com/chevydan6/haiku-intel-gpu/blob/9f273a18fe4ccc90b9a57b6e578c774d32656751/rondoflow-results/README.md).
- **N17:** [Dependency mapper](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/tools/map-dependencies.py); [strict repository audit](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/tools/audit-repository.py).
- **N18:** [Earlier provisional implementation decision](https://github.com/chevydan6/haiku-intel-gpu/blob/49a32ff126331d86fefe12c0a875936b6926eb2e/docs/IMPLEMENTATION_DECISION.md). Read with the newer mapping evidence, not as a reason to ignore it.

### Experimental branch and newer physical mapping

- **E1:** [LinuxKPI chronological progress, including the latest mapping checkpoint](https://github.com/chevydan6/haiku-intel-gpu/blob/3060587a5a28a7831049a858f933cdeadb25c06d/PORT_PROGRESS.md).
- **E2:** [Current experimental blockers and resolved failures](https://github.com/chevydan6/haiku-intel-gpu/blob/3060587a5a28a7831049a858f933cdeadb25c06d/PORT_BLOCKERS.md).
- **E3:** [Mapping implementation, source recipe and reproduction guide](https://github.com/chevydan6/haiku-intel-gpu/blob/3060587a5a28a7831049a858f933cdeadb25c06d/experimental/i915/mapping/README.md).
- **E4:** [Physical mapping evidence and installed-state record](https://github.com/chevydan6/haiku-intel-gpu/blob/3060587a5a28a7831049a858f933cdeadb25c06d/experimental/i915/evidence/physical-mapping-001/README.md).
- **E5:** [Actual private mapping ABI and owner implementation patch](https://github.com/chevydan6/haiku-intel-gpu/blob/3060587a5a28a7831049a858f933cdeadb25c06d/experimental/i915/mapping/owner-mapping.patch).
- **E6:** [LinuxKPI component Makefile](https://github.com/chevydan6/haiku-intel-gpu/blob/3060587a5a28a7831049a858f933cdeadb25c06d/experimental/i915/Makefile); [dependency inventory](https://github.com/chevydan6/haiku-intel-gpu/blob/3060587a5a28a7831049a858f933cdeadb25c06d/LINUXKPI_DEPENDENCIES.md).

**Maintenance rule:** update facts and evidence links when behavior changes; retain the implementation and failed-test history; do not turn this handoff into an additional approval layer.
