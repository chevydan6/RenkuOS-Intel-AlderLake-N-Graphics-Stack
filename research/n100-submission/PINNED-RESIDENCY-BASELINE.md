# Pinned applied source and residency inspection

Objective `b18f57bea5abecafa37f`, SOL generation 64. Source-only candidate for
independent verification; no milestone acceptance or runtime claim.

## Reproduction and identity

Run from the project root, selecting a new output directory:

```sh
GIT_NO_LAZY_FETCH=1 python3 tools/prepare-residency-baseline.py \
  --source /home/dan/projects/rondoflow-codex/.tools/renku-mission/source-cache/renkuos-pinned.git \
  --output tests/evidence/pinned-residency-baseline-064
```

The tool reads BASE and PATHS from production `tools/prepare.py` without executing
it; it adds the three required kernel paths and applies the actual sorted patch
directory with `git apply --check` followed by `git apply`. It deliberately skips
build preparation, tests copying and compilation. Existing outputs are refused.
`GIT_NO_LAZY_FETCH=1` applies to every Git subprocess; no fetch is invoked.

Evidence root:
`/home/dan/projects/rondoflow-codex/.tools/renku-mission/gpu-worktree/tests/evidence/pinned-residency-baseline-064/`.
`manifest.json` records exact command arguments, diagnostics, exit codes, pin,
archive hash, ordered patch hashes/results and six final file hashes.
`integrity.json` records a separate reread of every patch/final hash, actual patch
inventory, pinned kernel blob comparison and independent repair archive identity.
All 96 patches through 0096 applied; every command exited zero. Pin:
`4799bb5a188c4bb6bbef967287640cc21a061a0d`.
The archive is identical to the independently materialized repair archive.
Previous generation-063 evidence remains untouched. The bundle is local evidence,
not a replacement source checkout or an upstream source import.

## Applied production references

Paths below are relative to the evidence root's `source/`; these are final source
lines, superseding reliance on patch offsets for this inspection.

- `src/add-ons/kernel/bus_managers/intel_memory_owner/IntelMemoryOwner.cpp`:
  shared `sLock`/state/dependencies at 10–15; request lease/pin at 22–29;
  `Operation` at 53–83 brackets BeginOperation/EndOperation under that mutex,
  retaining failed allocation/free backing. `ReleaseDisplay` at 132–143 revokes
  admission and preserves mappings and in-flight pins. Observation admission at
  246–291 obtains the request lease and Query pin under sLock, copies device
  identity, unlocks for PCI capture, then rechecks state under sLock. Retirement
  at 233–242 retires bookkeeping only.
- The same owner file: `ObservationOperations` at 938–950 gets v0 on INIT and
  refuses UNINIT. `StandardOperations` at 954–978 gets PCI then GART, puts PCI
  only on failed GART acquisition, initializes the shared mutex once, and returns
  B_BUSY on UNINIT without cleanup. Exports at 981–1024 all use B_KEEP_LOADED.
- `headers/private/graphics/intel_extreme/IntelMemoryOwner.h`: v0 resource and
  callback ABI at 15–36, observation v1 at 38–49, v2–v8 at 51–110. No custody
  export or CPU backing interface exists in this applied series.
- `src/add-ons/kernel/drivers/graphics/intel_extreme/intel_extreme.cpp`:
  `release_aperture` at 630–648 revokes display and puts only the caller's owner
  reference; `intel_free_memory` at 661–665 and `intel_allocate_memory` at 670–677
  dispatch to resident v0 when present. `intel_extreme_init` at 692–707 selects
  exactly 8086:46d1/revision 0 and acquires v0 before begin_display.

## Comparison with accepted prospective design

Compare `RESIDENT-CUSTODY-DESIGN.md`, sections “Ownership and serialization
protocol” and “Kernel facilities and residency evidence,” against these exact
pinned files (kernel files remain byte-identical after patch application):

| Design claim | Pinned source evidence and finding |
| --- | --- |
| KEEP_LOADED retains owner code/image after successful INIT | `src/system/kernel/module.cpp:350–363,2216–2234`: successful B_OK INIT adds the permanent reference plus the caller reference; referenced modules retain image references. Matches design. INIT failure gets no permanent reference and releases the unused image. |
| Failed UNINIT retains rather than safely reloads | `module.cpp:798–821`: failure sets MODULE_ERROR, adds KEEP_LOADED and a reference; dependent-module release occurs only on success. `:763–766` rejects MODULE_ERROR initialization; `:2272–2280` preserves the image when failure leaves a reference. Matches the exceptional-refusal argument; B_BUSY is not a normal recovery protocol. |
| Dependency lifetime is resident | Owner `:959–975` explicitly retains successful PCI/GART gets; observations `:938–947` retain their successful v0 get. This relies on explicit unmatched successful references, not automatic propagation of the owner's KEEP_LOADED flag. Matches design, subject to no unsupported external unload. |
| Kernel mutex API supports shared admission | `headers/private/kernel/lock.h:21–27,142–156` declares mutex/init/lock/unlock. Owner `:62–72,252–291` uses them for state and executes PCI outside sLock. `module.cpp:365,2191` uses the module manager recursive lock: future get/put callbacks must stay outside sLock. Matches the proposed lock discipline; future callback bindings still require audit. |
| Interface and module declarations | `headers/os/drivers/module.h:18–29,38–48` defines module_info, B_KEEP_LOADED, dependencies, get/put. Matches the inspected design declarations. |

The earlier design's pinned-source equivalence gap is now resolved for these
three source files. No contrary residency semantics were found. This does not
establish equivalence to the installed kernel or matched runtime artifacts.
New custody-active admission, fixed custody records, trusted reserved-range
configuration, callback bindings and v9 remain prospective and unimplemented.
Forced unload/external deletion are unsupported. No compilation, runtime test,
hardware operation, canonical lock change or DMA address invention occurred.
Host source inspection cannot establish GPU execution or safe retirement;
uncertain GPU-reachable backing remains owned. Historical hardware findings,
A38–A40, recovery artifacts and all dirty baseline changes remain preserved.
