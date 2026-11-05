<!-- SPDX-License-Identifier: MIT -->
# GART lifecycle continuation — 2026-11-05 JST

The user requested continued autonomous development. Local work continued while
the power agent held OUTAGE_RESERVED for its physical baseline trial. No GPU
host, guest or NanoKVM jobs were started during that reservation.

## Unbind range

The generic manager adjusts both base and length to exclude the already-backed
prefix, but the unbind loop used the original length. For an allocation beginning
at page 2 with length 4 and a reserved prefix of 3 pages, the original code clears
pages 3–6 instead of pages 3–5, modifying a neighboring mapping.

Patch 9 changes the loop bound to the adjusted length. Verbatim production
methods are extracted into a C++ fixture with mock metadata/callbacks. On Linux,
ASan/UBSan passes all 46 checks after the fix. The original source reproduces six
expected failures across three crossing allocations. Fully reserved, ordinary,
end-of-aperture and first-page-failure cases remain covered.

Bundle `renku-016` contains nine patches, all 173 payload hashes verified, and
both positive/original controls pass their expected gates. The full generic
GART manager is added to the native build target; native validation is pending
the power agent's explicit restoration handoff. No physical driver was loaded.

## Next ownership problem

Partial `BindMemory()` success is not recorded until the entire operation
finishes. Cleanup can therefore free owned backing with PTEs still installed.
Additionally, `DeleteMemory()` ignores unbind failures. The next change must
track progress, retain owned pages and aperture metadata when cleanup fails,
propagate errors, and avoid indefinite teardown loops. External buffer lifetime
and pinning require separate treatment; retaining metadata alone cannot pin a
caller's backing pages. Fault-injected tests will distinguish these cases.

## Owned-backing lifecycle

Patch 10 tracks the installed prefix after every successful page bind. Unbinding
proceeds from its tail, updating the prefix only after each successful removal.
Any completed partial clear is flushed even when a later callback fails.
Deletion retains owned backing and metadata on failure; aperture unmap and
module teardown propagate cleanup failure, allowing a bounded retry without
destroying the aperture or looping indefinitely. Rebind and invalid size/state
requests leave existing mappings intact. New borrowed-buffer metadata is cleaned
up on failed binding when the backend permits rollback.

The fixture now executes verbatim production methods and public bind/unmap flow.
All **154** positive host checks pass under ASan/UBSan. The original code produces
**39** expected failures across **103** control checks; additional new-teardown
checks apply only to the patched code. Metadata allocation, page ownership/free,
registry primitives, destruction notifications and locks are simulated. This
does not validate real kernel locking, page allocation or GPU quiescence.
The caller keeps borrowed-buffer backing alive in these tests; kernel pinning
and rollback-failure lifetime contracts still need design before external GPU
buffers can be enabled. Allocation failure before binding and concurrent
aperture-reference lifetime also remain separate audit work.

Bundle `renku-017` contains ten patches and **176** verified payload hashes.
Both host suites and original-code gates pass. Native generic-manager compilation
is prepared but not run while the power agent's OUTAGE_RESERVED remains active.
The previously verified native eight-patch results remain distinct.

Kernel-loader review confirms failed module uninitialization retains the module
image/reference in `MODULE_ERROR` and rejects new acquisition. Thus the tested
retry behavior is allocation/aperture retry before such unload failure, not
automatic module recovery. No kernel-loader change was made.

## Allocation-failure follow-up

Source commit `21755757` (patch 11) releases the temporary pointer table when
the constrained physical-page run fails. The ownership flag has not yet been
set at this point, so ordinary `_Free()` previously skipped the leaked table.
The two-line fix also clears the stale pointer.

Extracted, unchanged production `AllocateMemory()` and `_Free()` bodies pass
**67 host sanitizer checks**, including contiguous/non-contiguous API requests,
table allocation failure, page allocation failure and successful cleanup.
The original code reproduces exactly three failures across 66 checks. The
fixture mocks VM/page allocation and tracks table ownership. Its test allocator
reclaims the detected original leak after recording failures so sanitizers can
still detect unrelated faults. This is not a native kernel allocator test.

Bundle `renku-019` contains eleven patches and **182** verified payload hashes.
Range/lifecycle checks still pass with the new extraction workflow. CI and the
native pipeline include the allocation test and original-source control. The
offline copy experiment is included as source but excluded from native driver
components. Native patches 9–11 await the explicit physical-test restoration
handoff; no host, VM or NanoKVM job was started during that reservation.

## Native validation after restoration

The Wi-Fi/power owner published OUTAGE_RESTORED and cleared the authoritative
marker. Fedora SSH and its task board were checked before claiming
`gpu-gart-native-lifecycle`. All guest work ran under host-config, haiku-vm and
build locks in a separate 768 MiB, two-vCPU, std-VGA VM. The existing Haiku VM
and host GPU were preserved.

First run `gpu-native-012` / bundle `renku-019` failed because the component
recipe omitted the platform kernel-header search path. The failure and clean
shutdown are preserved in `logs/renku-vm-012`. Adding the real SDK
`kernel/boot/platform/bios_ia32` include path resolved compilation.

Fresh run `gpu-native-013` / bundle `renku-020` passed with GCC 13.3.0 on
RenkuOS hrev60072+518 and its matching development package. All **182** payload
hashes verified. Complete generic GART manager, Intel GART, Intel kernel driver
and accelerant link; all three kernel artifacts depend only on `_KERNEL_`.

Native checks: 67 allocation, 46 range, 154 lifecycle, 185 hook lookup, 14 GGTT
range, 28 PTE, 65,537 GGC, 22 ring lifecycle and five gating checks. Original
controls reproduce the expected 3/6/39 allocation/range/lifetime failures and
21/3/2 hook/ring/gating failures. These are software fixtures, not kernel module
execution. The separate offline copy-packet experiment was not part of this
native run.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| Generic GART manager | 166336 | `057b802dbcc53c9bee1a9cfa68b5ca657a3ffae30c06da268ffb58cd5db38876` |
| Intel GART | 62984 | `bad6bd12d8a8d487b413e284bec8e0dffff47c4d79e4b4c7489f0eef81f87d7b` |
| Intel kernel driver | 243440 | `3b2841c50802f8648fd7e3a9fae7ccbe061e930909f221cc40f29a2fe392e657` |
| Intel accelerant | 884640 | `1bc52c8a26634bd7a3a0cfb08c5d43a525b675c6b7c13ac9be6fdb91a5e01fa1` |

Manifest SHA-256:
`4bcf7f6d1c5a183f9ed016c8e2d2717ba78dac02e615779afa175dddbd7742c0`.
Host artifact finalization passed. Coordinator
`astra-astra-gpu-98ac84db26f3.service` completed successfully in 146.709 seconds,
using 151.822 CPU seconds and 1.5 GiB peak cgroup memory, with zero swap recorded.
Guest shutdown was clean and QEMU exited zero. Evidence is retained locally in
`logs/renku-vm-013` and on Fedora in `testing/gpu-native-013`.

No module was installed or loaded. Hardware memory ownership, concurrency,
engine quiescence, command execution and graphics acceleration remain unproven.
