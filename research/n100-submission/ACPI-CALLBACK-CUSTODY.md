# ACPI callback custody

Objective f42988c03da1d0cd3457. Patch 0111 composes after 0100–0110 on
4799bb5a188c4bb6bbef967287640cc21a061a0d. AcpiOsExecute now calls the private
acpi_queue_callback production helper, which reserves a static record and both
producer/completion references under the admission spinlock before queue_dpc.

The 64-record image-owned pool performs no allocation, blocking, or heap release;
reservation exhaustion is the allocation-failure path. This is appropriate even
for interrupt-context AcpiOsExecute callers. The bound includes executing work
and producers whose callbacks already completed; saturation safely rejects work.
Both counters are checked before increment. Admission close linearizes against
reservation; previously reserved producers may subsequently publish, retaining
custody. The close return remains a producer snapshot, not a callback count.

The production completion wrapper invokes the original callback outside the lock,
then drops only its completion reference. Producer return drops its own reference;
queue rejection additionally drops unpublished completion custody. A record can
be reused only after both references are gone. Early completion cannot recycle it
under a returning producer. The 0100 DPC contract guarantees rejection precedes
publication and B_OK survives uncertain wakeup. Accepted queued or executing
callbacks remain tracked after admission closes, without replay or cleanup.

No waits, drains, DPC deletion, dependency release, public ABI change or weakening
of module final-lease quarantine. Zero callback counts do not prove image-wide
cessation: callback wrappers still return through image code, and other entry
paths exist. The image/dependency retention requirement is unchanged.

Host tests extract the actual composed BusManager production block. Real threads
exercise early completion, producer/close overlap and executing callback/close;
additional cases cover queued pool exhaustion, rejection, nesting, counter overflow
and post-close rejection. Existing actual DPC extraction checks wakeup uncertainty.
ASan/UBSan use detect_leaks=0 because traced LeakSanitizer is restricted. No native,
VM, hardware or GPU execution claim; independent TERRA verification is pending.
