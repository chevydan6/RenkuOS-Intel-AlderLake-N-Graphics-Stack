# Native page-state bridge for i915

Continue the authorized i915 adapter design using actual RenkuOS VM services.
The installed hrev60072+518 SDK and kernel successfully compile and export-link
vm_lookup_page, vm_cache_acquire_locked_page_cache and VMCache::Unlock. This
removes the uncertainty about reaching real dirty/accessed state.

Add a sleepable internal bridge operation on a physical page whose caller holds
a writable native pin for the entire operation. Acquire the page's real cache
lock/reference, require WiredCount() > 0, capture previous software modified and
accessed bits, and OR requested bits into the real vm_page. Release the reference
and cache lock on every acquired path. A zero mask queries these software flags.
Reject unknown bits, null output, unaligned physical address and disabled
interrupts before touching pages; leave output unchanged on error. The caller
must exclude all non-sleepable contexts, not just disabled interrupts.

Use an internal bridge API, not a Linux API claiming atomic-context support.
Deferred application from atomic Linux setters needs ownership, ordering and
newly-dirty return semantics; that integration remains a subsequent port step.
Neither this operation nor its query is a Linux dirty-page/writeback/LRU model.
The query does not harvest hardware mapping flags. No production clear operation
is exposed: clearing requires writeback and mapping synchronization contracts.

Keeping real VM state is preferable to synthetic successful setters, which can
lose modified backing. Importing Linux VM layouts would conflict with the native
memory manager. All original imported files remain byte-exact; new glue is MIT.

Native tests allocate only diagnostic-owned anonymous backing, hold its writable
pin, clear flags through a separate test helper using vm_clear_map_flags, and
inspect native fields independently of the bridge. Test dirty/accessed separately,
previous-state reporting, monotonic repeated/combined updates, invalid inputs,
disabled-interrupt rejection, and concurrent updates. Clearing is test-only and
limited to newly allocated private backing. Use the existing diagnostic module,
keep build/test logs visible through NanoKVM, verify real unload and unchanged
production components, and preserve fresh source hashes. No GPU test is claimed.

Risks: stale physical identity without a retained pin; native layout mismatch;
sleeping under compatibility locks; cache reference leaks; concurrent lost bits;
tests already starting dirty and therefore accepting a broken setter.
