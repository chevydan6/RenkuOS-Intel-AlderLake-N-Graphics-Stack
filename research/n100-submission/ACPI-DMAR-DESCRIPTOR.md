# Bounded DMAR descriptor provider

Objective `43d6884e97210ab40fd6`. Patch 0115 composes after 0114 on pinned
4799bb5a188c4bb6bbef967287640cc21a061a0d. The private v1 module adds an
independent ABI; existing ACPI structs and lookup semantics remain unchanged.
Its actual Module.cpp export uses acpi_module_std_ops, acquiring the core module
on init and releasing it on uninit, preserving dependency errors unchanged.
Call classify only while holding a successfully acquired provider module reference;
the acpicaStatus output pointer must be non-null. No pointer escapes.

Lifecycle Ready admission is checked under the lifecycle spinlock, released before
TABLES acquisition. Retained closure never terminates ACPICA or releases uncertain
image/dependency ownership, so a previously admitted operation retains custody.
NOT_READY performs no descriptor or ACPICA mutex access. MUTEX_FAILURE preserves
the exact ACPICA acquisition status and performs no release. Other outcomes set
AE_OK. A captured CurrentTableCount bounds the scan under TABLES; installed
Address and stored Signature alone classify zero matches as ABSENT, the second
as DUPLICATE, and exactly one as UNSUPPORTED regardless of origin. No backing
pointer, header, mapping, table reference, area or reserved configuration is used.
ABSENT does not authorize an empty reserved-range policy. Immutable backing,
trusted snapshots, callback cessation, DMA admission and GPU execution remain
unproven.

Reproduce: python3 tests/acpi-dmar-host/run.py <isolated-output>. The host harness
compiles extracted production classifier, lifecycle, provider initializer and
module std_ops. It exercises 11 schedules including dependency failure, unreadable
backing duplicates, three origins, mutex failure and repeated overlapping closure.
The runner also executes table, callback and DPC regressions. Interrupt closure
regressions were run separately. ASan/UBSan use detect_leaks=0 because of the known
traced LeakSanitizer restriction. These checks do not constitute native builds or
GPU execution. TERRA must independently compose and verify exact hashes and
production export/dependency behavior before ASTRA review.
