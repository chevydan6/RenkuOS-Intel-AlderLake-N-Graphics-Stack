# Managed-buffer ownership invariants (pre-implementation checkpoint)

This design is offline. It adds no registry, ioctl, device callback change, mapping or submission path to the running driver. The existing HDMI baseline remains intact.

## Separate the two contracts

The existing accelerant ABI intentionally shares the aperture and allocations across primary and cloned accelerants. Every `device_open()` currently returns the same `intel_info*`; raw-address allocation/free requests are device-wide. A new per-open owner check on those old requests would change that contract and could reject legitimate clone-driven mode changes. Preserve the legacy allocation/free behavior.

A future managed-buffer API must be distinct. Its objects belong to a per-open file context, rather than pretending the current shared `intel_info*` identifies an owner. Duplicated descriptors sharing the same file cookie share that owner; an independently opened descriptor gets a different owner. This is file-description ownership, not an assumption about process/team identity.

## Required invariants before callbacks change

1. Allocate a per-open context before publishing the cookie. Failure must leave `open_count`, hardware initialization and all existing opens consistent. Existing device state remains in `intel_info`; all legacy ioctls forward through the new cookie without ABI changes.
2. Managed handles contain no kernel or physical address. Lookup is confined to the caller's file context. Handles are monotonic and not reused during that context's lifetime; exhaustion fails rather than wrapping into a stale handle.
3. Reserve metadata before allocating backing. A successful allocation has exactly one tracked owner before any result is copied to user space. Never allocate cleanup metadata after an operation has already acquired resources.
4. Copyout failure is a failed ownership transfer. Attempt normal GART teardown. If it fails, retain the known allocation address and metadata as an undelivered object, available for a bounded retry/close cleanup; never silently lose the retry owner.
5. Destroy succeeds only after GART teardown succeeds. Failed teardown preserves both metadata and backing. Repeated destroy/cleanup is idempotent and must not double-free. Closing must not free pinned or otherwise live backing.
6. No userspace pin/unpin or arbitrary-address bind interface. Future job/display references are acquired inside the kernel under the object lock and held until proven completion/disable. Command submission remains absent.
7. Serialize owner lookup/state with a dedicated context mutex. Do not hold the global device-open lock over user copies or GART allocation/free. Define one lock order before introducing a device orphan queue; do not call back into owner locking while holding the GART aperture lock.
8. Preserve exact requested size and checked page-rounded backing size. Reject zero, unsupported flags, non-power-of-two alignment where required, overflow and out-of-range offset/length before allocation or lookup. The existing `size_t` to `uint32` accelerant narrowing must be checked separately.
9. No teardown silently discards live objects. If last-close cleanup fails, transfer its already allocated record to a device-owned retry/quarantine list before freeing the file context. The device/module lifetime must outlive that list; a void unload callback cannot simply discard it.
10. Do not advertise isolation or application-safe mapping while the trusted legacy accelerant can access shared raw aperture/MMIO. Managed object lifetimes are one prerequisite, not a complete security boundary.

## Existing backend gaps that constrain implementation

`gGART->allocate_memory()` returns its aperture address only after allocation and binding fully succeed. If binding fails and its own rollback fails, GART retains the partial object but does not return an address to the caller. A graphics-layer owner record cannot invent a retry handle for that failure. Distinguish this backend quarantine from an allocation that succeeded but whose descriptor delivery failed. Full recoverability requires a GART transaction/result extension or a backend-managed retry operation; do not change module ABI casually.

`intel_extreme_uninit()` currently ignores the `unmap_aperture()` status and deletes its register/shared mappings, and `uninit_driver()` frees `intel_info` through a void callback. A new device orphan list cannot be considered solved without an explicit retained-device/module lifetime or a teardown policy that is proven to work with the driver manager. This requires review before changing callbacks.

The public legacy `intel_free_memory(addr_t)` accelerant helper returns void and ignores failure. New managed-buffer destroy operations must return status and retain handles until release succeeds; do not copy this old convention.

## Smallest coherent future implementation

Implement an actual create/query/destroy managed-buffer path together with the per-open context, ownership records, normal/error close handling and tests. Do not add an unused registry in isolation. Keep the old ioctls unchanged and do not add submission or new physical mappings.

Before implementation, resolve how last-close cleanup failure keeps its owner/device alive and how backend partial-allocation quarantine is reported. Then test actual callback bodies with independent opens, duplicated-cookie access, stale/foreign handles, concurrent destroy, every copy fault, allocator/bind failures, partial teardown, close retry and module lifecycle. Native compilation precedes any coordinated hardware allocation test.

## Primary implementation references

- RenkuOS baseline `eeb334df`: `device.cpp` opens/free/ioctl dispatch; `intel_extreme.cpp` allocation/free/uninit; generic `agp_gart.cpp` tracked partial bind/unbind/delete lifecycle.
- [Linux Intel GGTT](https://github.com/torvalds/linux/blob/f66469d9905b39d2c260a15d7d6db450f4db67c1/drivers/gpu/drm/i915/gt/intel_ggtt.c), for the distinction between GGTT mapping/invalidation and object/job ownership. This is a reference, not a proposal to copy Linux's entire GEM/DRM infrastructure.

## Follow-up: retained GART state and reopen trace

Inspection plus an injected-backend test of the actual `map_aperture()` and `unmap_aperture()` function bodies establishes:

- `unmap_aperture()` holds the global manager lock and calls `DeleteAllMemory()` under the aperture lock. If that fails, it returns before removing the registry entry or invoking the aperture destructor.
- The aperture retains all remaining allocation metadata/backing, its backend pointer and its module reference. The backend `delete_aperture()` and `put_module()` calls occur only in `Aperture::~Aperture()`.
- The GART manager has `B_KEEP_LOADED`. Its own `agp_uninit()` also propagates deletion failure before deleting that aperture or dropping its PCI reference. Thus freeing graphics `intel_info` does not erase the retained GART backing/registry or unload its backend.
- A caller that still knows the old aperture ID can retry `unmap_aperture(oldID)`; after the simulated failure is removed, cleanup succeeds. Metadata is not intrinsically unrecoverable.
- Normal `map_aperture()` does not search for or retry an existing PCI aperture. It invokes backend `create_aperture()` and constructs a new registry object. The Intel backend uses global singleton `sInfo`; `intel_create_aperture()` currently overwrites its mapping fields and reinitializes non-reserved GGTT entries without checking for a surviving aperture. A reopen after failed unmap can therefore corrupt the backend state of the retained registry object.

The graphics driver loses its normal ownership/retry route when it overwrites or frees `intel_info`; the lower-level backing record survives. A future owner registry must not confuse those two facts.

`gart-retained-aperture-host.py` models only the production manager control flow, with simulated module references and a faulting singleton backend; it performs no PCI/MMIO operations and makes no concurrency or real module-unload claim. The test verifies retention, explicit old-ID retry, and reproduces duplicate backend creation during reopen.

A narrow proposed prerequisite is to refuse another Intel aperture creation while the existing singleton mapping remains active, before changing any mappings or GGTT entries. That would fail closed on reopen instead of corrupting retained state. Recovering/adopting the retained ID and a full managed-buffer API are separate changes; no such production guard has been added yet.
