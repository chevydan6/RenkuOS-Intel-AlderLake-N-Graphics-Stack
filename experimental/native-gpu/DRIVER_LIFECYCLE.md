<!-- SPDX-License-Identifier: MIT -->
# Native BCS driver lifecycle

The owner and legacy bridge now use the private `native_execution/v3` module;
replace both together and reboot. Existing public buffer/copy ioctls are unchanged.
The additional fixed-width API is in `LifecycleReport.h`. No user GPU commands,
addresses, mappings or callbacks are accepted. This work changes the GPU driver
and bridge, not the RenkuOS kernel, ACPI, display driver or accelerant.

## Requests, clients and memory

One resident request can execute at a time. Competing clients wait outside the
device lock for at most 250 ms, sleeping between admission attempts. A busy timeout,
interrupted waiter or unavailable device rejects the call before publication.
The sequence advances only on admission. CPU access within one open is serialized
with its synchronous copy; separate opens have separate buffers and patterns.

Every accepted exit records success or failure. Successful retirement still
requires the matching CSB events, sequence/nonce, visibility, complete destination
verification, idle/error checks, normal power release and final owner/DMA checks.
Earlier bootstrap/allocation failures can no longer strand an ambiguous busy flag.
Status distinguishes a running request from Lost and terminal Shutdown.

Each open holds a reference to the resident module through free-cookie. Close
waits on the session mutex and rejects subsequent operations. The pinned RenkuOS
[`fd_ioctl`/`put_fd` implementation](https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/system/kernel/fs/fd.cpp)
holds an ioctl's file reference through return and calls free only after the last
reference. Thus free-cookie can release CPU staging and the module reference
without racing an active ioctl. Killing a client does not free resident DMA pages.

Pre-admission rejection leaves CPU buffer ownership unchanged. An admitted failed
copy leaves that client's participating buffers Unknown, even after device
recovery; the client must close and create fresh staging. No failed copy is copied
back to the client. The fixed resident GPU pool remains pinned for the boot,
including its context, ring, HWSP, page tables and two data pages. This is bounded
pool reuse, not a general allocation/free API or last-client GPU deallocation.

## Stalls and device errors

CSB waiting has a 20 ms ceiling and reserves at least 20 ms of the forcewake operation
budget for recovery. All waits also retain iteration and owner/clock checks.
Timeout returns `B_TIMED_OUT`; unsafe/error completion returns an error. A valid
copy whose completion was not established is never promoted to a successful copy.

Recovery runs inside the still-held forcewake group. `BcsRecovery.h` adapts Intel's
Linux v6.6 stop-CS, reset prepare/cancel and domain-reset logic: STOP_RING, Gen12
prefetch disable, idle/posting read, pending MI forcewake check, READY handshake
(or catastrophic-error bypass), two BCS0 domain resets and 50 us settling. The
strict cold reset helper remains unchanged. Non-ready forced retry and full-GT
reset are not added.

Reuse requires disabled/idle engine observation after reset, no persistent engine
or shared fault, normal forcewake release and current owner/PCI/DMA validation.
Then the generation advances; the next request rebuilds context/HWSP/engine state
before publication. Reset does not free backing. A failed reset, changed owner,
enabled VT-d translation, persistent device error or failed power release enters
Lost and retains memory until reboot. This BCS owner does not clear shared GT
faults or reset display/render engines to conceal another engine's error.

## Driver power boundaries

`build/gpu_lifecycle` queries status or invokes an administrative action. Controls
require effective UID0; query and ordinary owned-buffer copying do not.

| Command | Driver behavior |
| --- | --- |
| `status` | Snapshot state, client count, sequence, generation, counters and last request timings. The querying open is included in client count. |
| `quiesce` | Freeze admission before waiting up to 250 ms for the active request to finish or recover. Success reaches Suspended. |
| `resume` | From Suspended only: revalidate PCI/DMA, acquire power and reset an initialized engine, release power, then reopen admission. Rebuild context on the next copy. |
| `shutdown` | Freeze admission, drain, then enter terminal Shutdown. Resume cannot reopen it; reboot is required. |
| `test-timeout` | Arm one global administrative check: hide CSB completion observation for the next valid copy, exercising bounded timeout and real BCS reset without malformed GPU commands. |

State numbers are Cold=0, Ready=1, Running=2, Quiescing=3, Suspended=4, Lost=5,
Shutdown=6. Counters count accepted requests, not rejected calls. Timings are
microseconds; cache time measures the current whole-machine `wbinvd` path.

These are **driver boundaries**, not OS S3 support. At the user's explicit scope
choice, no RenkuOS kernel/ACPI changes are included. The pinned legacy driver API
has no power hooks; inspected device-manager suspend/resume callbacks are not
dispatched, and ACPI's sleep entry does not implement a returning wake path.
Use the driver quiesce/shutdown control before an external power transition;
resume must be invoked after returning to a usable D0 device. The exported
`gKernelShutdown` flag additionally blocks new GPU admission/publication once OS
shutdown starts. It is not an OS callback that waits for driver drain.

## Completion and scope

Polling remains intentional: native warm requests consumed one CSB poll and no
sleeps. Measured wait was about 120–148 us, versus 1397–1432 us of cache maintenance
and 609–648 us of submission on this small-copy path. Interrupt integration offers
no demonstrated benefit here and would require coordinated GT interrupt identity,
bank acknowledgment and shared master/MSI ownership with the Intel display driver.
It must not be implemented by independently unmasking BCS interrupts. IRQs, if
later justified, notify the waiter; they do not replace retirement checks.

Actual native evidence and exact limits are recorded in
[native-lifecycle-003](evidence/native-lifecycle-003/README.md). The timeout check
does not prove recovery from a physically hung engine, and no actual device fault,
OS suspend/wake or hardware power-loss transition is claimed. The observed
VT-d-translation-disabled restriction remains; general IOMMU work is separate.
