# Reserved unlocked UNINIT audit

Objective 05ad6ff6be2ed0759bbd. Pending independent TERRA verification and ASTRA acceptance.

Pinned module.cpp 4799bb5a188c4bb6bbef967287640cc21a061a0d composed with patches 0100–0110. Actual callers remain ACPI Module.cpp acpi_module_std_ops and BusManager.cpp acpi_std_ops.

put_module_internal rejects caller-owned lock depth before entry allocation or mutation. Its last-reference decrement and uninit_module reservation execute under the same depth-one lock without intervening unlock or allocation. Transition-active metadata plus image transition_pins replace the external reference before callback entry. Reservation failure preflight leaves references unconsumed; callback failures and stale identities consume into retained custody.

UNINIT uses the ticket's callback at unowned depth. Reacquisition precedes validation of target, image, info, generation, state and per-thread context identity before any dependency-lease mutation or final release. Mismatch and callback failure preserve original image custody, quarantine and no replay. Existing final-lease quarantine and conservative ACPI UNINIT are unchanged. No admission drain, termination, DPC join or dependency cleanup is enabled.

Extracted production-function tests exercise pre-owned rejection, allocation failure, stale identities, callback failure, no replay and real ACPI nesting. The real-thread fixture adds competing last puts, target get busy, unrelated progress and nested release during paused unlocked UNINIT. ASAN_OPTIONS=detect_leaks=0 disables leak checking for the known traced-process restriction. No full kernel build, native loader, VM, hardware, deployment or GPU execution is claimed.
