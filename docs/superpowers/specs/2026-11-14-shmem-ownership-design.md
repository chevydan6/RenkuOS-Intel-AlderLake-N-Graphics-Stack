# Native shmem ownership for the original i915 port

The accepted handoff requires Linux file/address-space/shmem ownership over the
native pageable store. This change executes that part of the existing port;
the full objective remains original i915 compilation and real GPU execution.

The existing adapter design maps Linux interfaces onto RenkuOS services while
keeping imported algorithms byte-exact. Continue that design. Importing Linux's
entire VFS/VM would require a second kernel memory manager; inventing successful
page-cache or reclaim operations would hide missing driver requirements.

Implement scoped `struct file`, `inode` and `address_space` views owned by a
native shmem object, not binary copies of Linux VM layouts. `shmem_file_setup`
creates a real anonymous native area and links the three objects. `get_file`
and `fput` use the original Linux refcount implementation. A final file put
closes the backing to new reads but keeps mapping metadata and the native area
alive while returned folios or in-flight pins exist. Final unpin and area
deletion run on the existing joined page-release worker. Failed area deletion
retains ownership and retries; drain/uninit must never report completed cleanup
while an area or release callback remains.

Preserve the existing explicit store-destroy API and failure-retry behavior.
Add owned-store get/put and a once-only release callback for shmem metadata.
An unpublished store can select native no-commit backing for VM_NORESERVE,
using CREATE_AREA_DONT_COMMIT_MEMORY from the actual pinned SDK. A zero-size
file has no readable page; unaligned logical sizes round the backing safely.

`shmem_read_folio_gfp` and `shmem_read_mapping_page_gfp` return real referenced
managed folios/pages and Linux error pointers. EOF, size overflow, unknown
flags, allocation failure and constrained allocation contexts must fail
explicitly. This checkpoint supports ordinary sleepable GFP_KERNEL allocation;
it does not pretend that native faulting honors arbitrary Linux no-reclaim,
zone, retry or filesystem-recursion constraints. Mapping GFP mask helpers
preserve caller constraints rather than silently broadening them.

Dirty/accessed state, writeback/truncate, unevictability and user mapping methods
remain unresolved until their native contracts are implemented. No successful
no-op is added. This checkpoint is not full shmem compatibility or a GPU result.

Tests exercise resource lifetimes, zero-fill/retention, reference sharing,
close-with-pins, concurrent readers, failure unwind, no-reserve policy and
unsupported GFP rejection against the real C adapters and a host RAM backend.
Native tests must then compile the C++ bridge, observe actual area lifetime,
run existing completion/VMA regressions and verify unload/baseline preservation.

Key review risks are close-vs-pin races, callback execution under a spinlock,
duplicate queueing, lost store references, delete failure and timeout behavior,
rounding overflow, and false allocation-policy claims.
