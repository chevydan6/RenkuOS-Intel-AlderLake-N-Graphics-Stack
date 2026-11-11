# ACPI producer admission host regression

Run `python3 tests/acpi-admission-host/run.py /absolute/isolated/output`.
The runner copies five files from pinned source
4799bb5a188c4bb6bbef967287640cc21a061a0d and applies patches 0100 then 0101.
It extracts the actual admission/initialization functions and AcpiOsExecute;
the C++ harness supplies deterministic host kernel and DPC shims. Eleven cases
cover initialization, closure before lock acquisition, closure while leased,
rejection, ambiguous/null creation, close during construction and exhaustion.
Rejected acquisitions must leave output pointers unchanged. Queue entry and its
end-of-call wakeup boundary require an outstanding lease and no admission lock.
The separate thirteen-case production DPC suite tests actual wake failure and
truthful accepted ownership; the ACPI shim models its B_OK result. This is layered
host coverage, not a combined native ACPI/DPC execution or GPU execution proof.
ASan/UBSan run with ASAN_OPTIONS=detect_leaks=0 because traced LeakSanitizer is
restricted. The runner records commands, exits, composed source hashes and output.

Admission storage is image owned. External manager image/dependency custody is
still an unresolved prerequisite. Existing AcpiTerminate, failure cleanup and
UNINIT/delete paths remain unsafe and unchanged. No closure is wired to teardown;
zero leases cannot establish callback cessation or permission to unload/release.
Do not deploy this patch until those independent lifecycle gates are satisfied.
