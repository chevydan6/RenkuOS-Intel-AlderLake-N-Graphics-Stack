# Trusted DMAR configuration: pinned provider contract

Objective `a993192d67202bdcf6b9`, SOL generation 67. Design only, ready for
independent verification, not accepted. Parent b67a71e984eca7c002c284d8dc3110ea21fe9428.
No compilation, production implementation or hardware operations occurred.

## Exact source finding

Pin: `4799bb5a188c4bb6bbef967287640cc21a061a0d`. Absolute evidence root:
`/home/dan/projects/rondoflow-codex/.tools/renku-mission/gpu-worktree/tests/evidence/trusted-dmar-contract-067`.
`sources.json` supplies SHA-256 and Git blob identities for every pinned file;
`pinned/` contains the inspected original bytes for independent line checking.
Paths in this section are relative to the pinned source root.

* `headers/os/drivers/ACPI.h:294`: acpi_module_info::get_table returns status
  and void pointer, with neither extent nor table-release operation.
* `src/add-ons/kernel/bus_managers/acpi/BusManager.cpp:851–855` directly wraps
  AcpiGetTable, translating **all** non-AE_OK results to B_ERROR. Its
  gACPIModule uses B_KEEP_LOADED. `Module.cpp:344` also binds get_table.
* `acpica/components/tables/tbxface.c:366–417` (under that bus manager) scans
  matching signatures under ACPI_MTX_TABLES. Instances 0 and 1 both select the
  first match; 2 selects the second. It returns AE_NOT_FOUND only when no match
  exists, but the public wrapper loses this distinction from AE_NO_MEMORY.
* `acpica/components/tables/tbutils.c:425–466` increments descriptor
  ValidationCount and returns the validated pointer. `tbdata.c:179–219,375–400`
  maps the descriptor's recorded Length for physical origin, or uses a virtual
  pointer. The public API does not expose that recorded extent. Mapping success
  is not proof that mutable header Length still matches the descriptor length.
* `tbxface.c:422–473` provides internal AcpiPutTable and explicitly warns about
  table uninstallation; this operation is absent from acpi_module_info.
  Module image retention alone does not prove firmware bytes immutable or rule
  out table descriptor removal. A copied header's length alone is no extent proof.
* Real callers: `src/add-ons/kernel/drivers/graphics/radeon_hd/radeon_hd.cpp:56–79`
  gets ACPI, requests VFCT instance 0, checks header length, and puts the module
  on failure. `src/add-ons/kernel/drivers/timer/hpet.cpp:359–369` similarly gets
  HPET instance 0. Neither establishes bounded unique DMAR admission.

Consequently the existing public API cannot prove this objective's complete
contract. In particular B_ERROR for instance 2 cannot mean “unique”: it could
be a mapping failure for a duplicate. Do not configure custody through it.
No pinned paths above are missing; the missing facility is a public bounded,
status-preserving table snapshot operation with descriptor extent and lifetime
protection. No alternate baseline or successful runtime equivalence is asserted.

## Proposed provider and integration boundary (unimplemented)

A separately versioned ACPI provider operation should return a status-tagged
snapshot: FoundUnique, Absent, Duplicate, Unsupported, NoMemory, DependencyError,
or InvalidTable, plus copied length and bytes on success only. Preserve existing
ACPI ABI. Use a caller-owned fixed 4096-byte buffer and no escaped table pointer.
Inside the ACPI provider, hold its table-list protection over selection and copy;
inspect the trusted descriptor extent before reading even the full header.
Enumerate matching DMAR descriptors, stop on the second (Duplicate), using the
finite installed table list, not an unbounded caller loop. Distinguish descriptor
presence from mapping success. Propagate lookup/mapping failure; never count an
unreadable duplicate as absence. The scan is bounded by CurrentTableCount at
entry under the table mutex. Reject concurrent installation/uninstallation by
that same protection; audit all mutators before claiming it suffices.

Require extent >= 48; read length only within accessible header bytes; require
48 <= declared length <= 4096 and <= descriptor extent, and agreement with the
recorded table length for this contract. Acquire/map only under the provider's
supported table reference protocol; retain that reference until copy completes.
Use existing ACPICA get/put internals only inside their owning provider, never
link private symbols from intel_memory_owner. Release only a completed temporary
reference, outside owner sLock; never release CPU/GPU backing on uncertainty.
Revalidate signature, exact copied extent and checksum on copied bytes, then use
actual intel_dmar_reserved, not a duplicate parser. Unsupported records/scopes,
flags and capacity overflow fail closed. Header revision/reserved-field support
must be explicitly specified by the future adapter: current parser does not
validate all such fields; unsupported semantics must not be silently accepted.

Do not add ACPI dependency failure to v0 StandardOperations INIT: applied
IntelMemoryOwner.cpp:954–978 currently gets PCI/GART and initializes sLock; changing
that would regress display initialization. Existing intel_extreme.cpp:630,661,670
release/free/allocate dispatch stays unchanged. Prospective custody admission
lazily acquires the new ACPI provider outside sLock, after reserving its existing
lease/pin and a Configuring gate under sLock. The gate excludes observation and
other custody configuration; close invalidates publication without freeing the
stable record. Record successful module references before invoking provider code.
No get/put, allocation, table callback or wait under sLock; no callback reentry.
Keep successful provider dependencies resident with the owner on uncertain state.
A definite no-resource initialization failure may put only proven unused refs.

Copy into stable module-owned staging storage outside sLock; run validation and
ConfigureReserved before BeginCpuBacking. Recheck gate, generation and close state
under sLock before committing Configured. Only successful configuration enables
area acquisition; no create_area callback on any preceding failure. Configure
exactly once per admitted record, including after EndCpuBacking: reject second
configuration at the adapter gate even though KernelPages can allow it when
inactive/empty. Reject active/live/ambiguous reconfiguration without mutation.
The staged buffer can expire after ConfigureReserved copies its range value.
Keep configuration provenance (pin/provider identity, copied length/digest and
range count) with the record; a zero-initialized Range struct is no provenance.

## Independently checkable acceptance cases for later implementation

These are required cases, not executed tests. Exercise the actual future ACPI
snapshot operation and resident adapter with controlled external facilities,
and actual IntelDmarReserved/Inventory and KernelPages production helpers.

| Input or interleaving | Required observation |
| --- | --- |
| No matching descriptor | Absent; unconfigured; zero area calls |
| Two DMAR descriptors, including unreadable second | Duplicate or explicit dependency failure; never unique; bounded scan |
| Mapping/module/allocation failure at each boundary | Distinct failure preserved; unchanged configuration; zero area calls; exact reference accounting |
| Short extent, length 47/4097, extent/header disagreement, signature/checksum error | Reject before out-of-extent read or ConfigureReserved success |
| Supported table with no RMRR, e.g. supported DRHD only | Actual parser succeeds with count 0 and trusted snapshot provenance; permitted CPU filter, no DMA admission |
| Missing bytes or invented empty policy | Reject; cannot substitute all-zero ranges for successful firmware validation |
| Unknown structure/scope, invalid flags, malformed RMRR, 17th range/unit | Actual parser rejects and output/configuration unchanged |
| Revision/reserved header fields outside reviewed support | Adapter rejects even where current parser would accept |
| Successful unique snapshot | Copy length exact; checksum/parser run on copy; no pointer escapes; reference spans copy |
| Table mutation/uninstall during selection/copy | Provider synchronization prevents it or explicit failure; no torn snapshot accepted |
| Close/stale generation during callback, nested admission | Retain stable context/dependencies; no Configured publication or area calls; no sLock in callback |
| Configuration after activation, live area, ambiguity, or prior configuration | Reject without altering policy/backing; no cleanup |
| ACPI unavailable during ordinary v0 display INIT | Existing PCI/GART display initialization unaffected; custody alone unavailable |

Source inspection is the only check in this task. Host/VM tests cannot establish
GPU execution. Future sanitizer runs require ASAN_OPTIONS=detect_leaks=0 under
the known traced LeakSanitizer restriction and must report disabled leak checks.
Installed kernel/artifact equivalence, provider synchronization audit and the
versioned provider implementation remain prerequisites. No deployment, replay,
reset, reboot or canonical lock changes are authorized; uncertain DMA backing
stays owned. A38–A40 and historical recovery evidence remain untouched.
