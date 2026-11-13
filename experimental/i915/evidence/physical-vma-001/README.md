<!-- SPDX-License-Identifier: MIT -->
# Complete DRM VMA module on the N100

The byte-exact Linux v6.6 DRM VMA offset manager builds with original drm_mm and
rbtree, plus native OS adapters. All 34 imported blob IDs were checked against
the pinned Linux tree as well as against the repository manifest. Source checking,
native GCC13.3/SDK518 compilation and tests ran visibly in fullscreen Terminal
through NanoKVM on RenkuOS hrev60072+518. SSH transferred files/collected evidence.

Four invocations pass 646 checks each (2,584 checks, zero failures) and 12,000
reader/writer operations each (48,000 total). Two reader workers overlap under
the native rw_spinlock; the writer sees no active readers and readers see no
torn writer state. Native IRQ exclusion/restoration and nested lock scopes pass.
Range exhaustion/interior/exact lookup, permission reference counts/allow-once,
revocation, zeroed allocation and IRQ/atomic allocation rejection also pass.
The original log2 header is exercised at every bit boundary including bit63.

Actual diagnostic unload returns zero, deleting its wired per-CPU scope table.
The runner removes its exact driver/link and verifies unchanged graphics devices.
Native deferred_free handles heap objects released with IRQs disabled; freeing is
asynchronous in the OS daemon, not a module-owned callback. No PCI/GT registers,
GPU commands or user-page mapping operations are involved in these tests.

build-native.log is warning-free under Werror. payload.sha256 and binaries.sha256
identify the tested sources/artifacts. result.json identifies the full source
payload, raw native result archive and screenshot outside Git.

The full VMA translation unit also passes the host cross-source build using real
RenkuOS headers (whole-vma-final.log). The existing allocator/refcount/ww/emitter
sanitizer regressions and target-header atomic/emitter probes pass; host DMA API
regressions pass separately. Host VMA OOM and mutation controls are in vma-host-001.

The complete original GEM and ring files still fail, with actual compiler exit
codes retained here. Initial annotation/stdarg/log2/gfp_t and rwlock errors are
resolved. Later type aliases, architecture arithmetic, lockdep/raw-spin/wait
interfaces and missing files in the external sparse Linux checkout remain.
A complete i915 driver does not yet compile or run.
