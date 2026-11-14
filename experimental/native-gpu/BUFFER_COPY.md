<!-- SPDX-License-Identifier: MIT -->
# Narrow owned-buffer copy interface

The existing `/dev/misc/intel_native_execution` endpoint now accepts the fixed-width
ioctls in `BufferCopy.h`. The resident private module is `native_execution/v2`;
replace both owner and bridge, then reboot. The original fixed-copy ioctl and
`gpu_copy_sample` remain available. No user command stream, GPU address, physical
address, mapping or callback is accepted.

Each successful open owns a serialized session with at most two 4096-byte CPU
staging buffers. `NATIVE_GPU_BUFFER_CREATE` takes `{bytes=4096,handle=0}` and returns
a handle in that open-file namespace; new buffers are zeroed. Handles cannot be
transferred to a separate open. Sharing/duplicating the file descriptor shares
the session. Buffers live until final file-cookie release; there is no individual
destroy or reuse operation in this first interface.

`NATIVE_GPU_BUFFER_WRITE` and `NATIVE_GPU_BUFFER_READ` take `native_gpu_buffer_io`:
handle, byte offset, positive byte count and an inline4096-byte payload. The range
must fit the owned buffer. These are copies through kernel memory, not mmap or
zero-copy DMA. Read returns the requested bytes at payload offset0 and zeroes the
unused payload. User memory is accessed only through `user_memcpy`.

`NATIVE_GPU_BUFFER_COPY` accepts `native_gpu_copy`: source/destination handles,
width and height in32-bit pixels, and independent source/destination byte pitches.
Both origins are0. Buffers must be distinct, created in this session and CPU-owned.
Zero dimensions, coordinates/pitches outside positive signed16-bit values,
non-dword pitches, row width exceeding pitch, and a final row exceeding4096bytes
are rejected before submission. Bounds use64-bit arithmetic. Distinct handles
select separate staging arrays; execution always uses two distinct owned private
PPGTT pages, avoiding physical aliases as well as overlapping GPU VA ranges.
Unsupported requests are rejected rather than clipped or split.

Submission is synchronous and polled with one global GPU request in flight. Its
sequence, data proof and CSB retirement are shared with the repeated fixed copy.
On success the copy ioctl returns its execution report. Whole-page hashes may
differ for rectangular copies with padding or different pitches; `copy_words`
and `copy_matched` describe the rectangle. The sample also checks the returned
bytes. Run `build/buffer_copy_sample` after building/installing the native bundle.

## Visibility and lifetime

The bridge holds the session mutex through user copies, staging access, the
resident module call and report return. CPU reads/writes and other requests in
the same session serialize. The state progression is:

1. **CpuOwned:** CPU reads/writes are allowed. Validate ownership and geometry.
2. **GpuInFlight:** snapshot both CPU staging buffers into the resident owner's
   pinned source/destination pages. Preserve destination bytes outside the
   rectangle. Only these resident pages, never the staging arrays, are DMA-visible.
   Flush WB data/page tables and WC command stores with the existing x86
   `mfence; wbinvd; mfence` contract before publication. Intel packet construction,
   MOCS/cache policy, FORCE_RESTORE and ordered completion remain in the kernel.
3. **Retire:** accept the current-tag CSB activation and idle switch-away, exact
   ordered breadcrumb and nonce/complement, refresh CPU visibility, verify the
   copied rectangle and every untouched destination word, then check engine idle,
   errors, owner and DMA admission. Release power normally and revalidate.
4. **CpuOwned:** only after full success copy the resident destination back into
   the session buffer. The owner releases its admission gate for another request.
   A rejected global-busy call did not publish this request and leaves the CPU
   staging buffers usable. Other execution failures mark the participating
   session buffers **Unknown**, making reads/writes/copies unavailable.

An uncertain GPU completion keeps the global admission gate closed and retains
all resident DMA backing until reboot. Session buffers are CPU-only snapshots;
closing their file can free them even on failure because no GPU or resident
callback/pointer references them. The bridge and resident owner retain no request
pointer after the synchronous call. The owner's global request storage, mappings,
PPGTT and source/destination backing stay pinned and are reused only on success.
This interface deliberately does not reclaim published GPU backing on close.

## DMA scope

This remains the observed N100 direct-DMA backend. Admission verifies the current
PCI identity, ownership and DMAR coverage, checks each enumerated VT-d remapping
unit has translation disabled, and excludes reserved physical ranges. Translation
status is rechecked during execution. It never disables an enabled IOMMU or
assumes its mappings are identity mappings. General IOMMU mapping/admission,
shared buffers, arbitrary allocation sizes, offsets, tiled formats and asynchronous
submission are separate work.
