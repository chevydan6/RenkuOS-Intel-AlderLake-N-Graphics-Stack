# DPC ownership primitive — candidate for independent verification

Objective 51412ee329d0ebc02191; SOL generation 97. Patch
`patches/0100-dpc-truthful-ownership.patch` applies to production revision
4799bb5a188c4bb6bbef967287640cc21a061a0d. Public module layout remains v1.
No independent acceptance, native shutdown, deployment or GPU execution is claimed.

Publication transfers callback ownership; a later wakeup error returns B_OK and
sets sticky uncertainty. Rejections precede publication. Queued, executing and
wakeup-in-flight counters and closure transitions use the IRQ-safe queue lock;
callbacks execute unlocked. Self-delete rejects before mutation; a single closer
owns deletion. Closure overlapping a producer wakeup quarantines without deleting
the semaphore. Free requires successful semaphore deletion, successful original
worker join, B_OK exit, empty queued/executing counters and no uncertainty.
Failed start and failed cleanup after spawn failure return retained handles even
on error. Uncertain numeric IDs are never replayed. Strict compilation also
corrected the original semaphore-name precision to reserve the terminating byte.

External handle/image leases must cover API entry, including a producer paused
before the queue lock. Successful close requires exclusive lifecycle custody and
no outstanding producers. Internal synchronization cannot protect a raw pointer
after free. Concurrent-delete rejection is only meaningful while external
custody keeps the queue alive; the test keeps the first closer blocked on an
executing callback. Every uncertain path retains queue, callback referents,
module images and dependencies. No ACPI destructive teardown is enabled.

## Pinned consumer audit and integration blockers

Search of src and headers at the pinned revision is preserved in
`tests/evidence/dpc-ownership-096/all-callers.txt`; exact per-call context is in
`caller-contexts.txt` beside it. All discovered API calls are covered below.
These findings prevent runtime integration; none of these consumers is certified
compatible merely because its normal path can drain.

| Consumer and exact file references (relative to src/add-ons/kernel) | Ownership and deletion assumptions requiring integration |
| --- | --- |
| bus_managers/acpi/ACPICAHaiku.cpp:876; BusManager.cpp:208,286,297; Module.cpp:36 | Execute reports rejection without owning a producer/image lease. Truthful publication now preserves accepted callbacks on wake failure. Creation failure returns without handling a retained non-NULL handle. Both deletion sites ignore errors and clear the handle; termination ordering and module dependency custody remain blockers. |
| bus_managers/firewire/fwohci.cpp:769,822,1889,1941,1947; firewire_module.cpp:232,245–252 | Creation and interrupt enqueue statuses are ignored. Detach ignores deletion errors and clears taskqueue, while module uninit frees controller records and puts DPC. No demonstrated interrupt-entry lease or retained failure custody. |
| bus_managers/hyperv/VMBus.cpp:98,142,602; Driver.cpp:21 | Creation status alone cannot establish cleanup after ambiguous start. Interrupt enqueue ignores rejection; destructor ignores deletion errors then destroys locks/areas/fCPUData referenced by callbacks. Disabling interrupts does not itself prove all entered producers ceased. |
| bus_managers/hyperv/VMBusDevice.cpp:57,108,144,315 | Destructor destroys fLock before deleting; deletion result ignored. Close frees GPADL then ignores deletion failure and clears handle. Callback passes object pointer without proven lifecycle lease; failed creation may retain a worker. Repeated close/destructor must not replay uncertain IDs. |
| busses/random/ccp/ccp.cpp:45,88,103,117 | Timer/initial enqueue ignore result. Creation failure does not handle retained queue. Teardown passes &bus->dpcHandle instead of the queue handle, ignores deletion outcome, then deletes register area. cancel_timer alone supplies no demonstrated in-flight producer custody. |
| busses/random/virtio/VirtioRNGDevice.cpp:67,82,89,168; virtio_rng.cpp:111 | Timer and immediate enqueue ignore rejection. Constructor error can retain worker. Destructor cancels timer then ignores deletion failure while object lifetime ends; callback references this and its device dependencies. Timer cancellation and interrupt entry require explicit cessation evidence. |
| drivers/ports/pc_serial/Driver.cpp:529,561,602,653,570,607 | Interrupt enqueue checks rejection, compatible with truthful publication, but carries a device pointer without proven lease. Init failure jumps to module release even if creation retained a handle. Both delete sites ignore errors/clear handle; unload frees records and releases DPC/dependencies. Cannot treat deletion failure as cessation. |

## Bounded host checks

`tests/dpc-ownership-host/run.py <patched dpc.c> <output directory>` compiles and
includes the actual patched production translation unit with the actual patched
public header. Kernel shims supply real pthread workers, spinlocks and condition
variables; barriers control worker start, callback execution and closure. No
replacement queue state machine is used. Thirteen isolated processes cover normal
drain, full/null rejection, accepted wake failure, wakeup-in-flight versus close,
failed start, spawn cleanup failure, semaphore-delete failure, join failure,
invalid worker exit, executing callback plus concurrent closer, callback requeue,
self-delete, and acquire failure. Retained queues are accessed to detect accidental
free under ASan; uncertain workers remain owned until process exit. Successful
join paths also check exact callback counts. Stub IDs deliberately remain fixed;
repeat delete assertions check no semaphore or join replay.

All thirteen passed ASan/UBSan with strict warnings. LeakSanitizer is disabled via
ASAN_OPTIONS=detect_leaks=0 due to the known traced-process restriction. This
harness establishes host production-function behavior only, not native scheduler,
interrupt, module unload or GPU behavior. Exact commands/results, applicability,
source identities, preservation and candidate metadata are under
`tests/evidence/dpc-ownership-097/`. TERRA must reproduce in its isolated directory,
inspect the patch and caller leases, and report a verdict before ASTRA review.
