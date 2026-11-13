<!-- SPDX-License-Identifier: MIT -->
# Persistent backing and pin ownership; native validation pending

Final host003 and the restored final executable pass 49,504 ASan/UBSan checks.
The production C page/folio registry is tested with real host RAM and pthread/
semaphore synchronization. Independent backend pin accounting rejects leaked
pins and deletion while pinned. The model deliberately changes synthetic physical
identity on re-pin after a complete unpin while preserving contents. This is
not native physical mapping, migration, paging, swap or DMA evidence.

The internal store API retains anonymous backing independently of folio pins.
Tests cover a five-page non-power-of-two store, zeroed untouched pages, whole-page
data retention, canonical repeated reads, two simultaneous first-pin operations,
rollback of the redundant native pin and two users/256 read-map-ref iterations.
Deletion refuses active folios, blocked in-flight pins and queued unpins; even
empty stores prevent backend shutdown. Pin failure leaves no ownership. Failed
area deletion preserves the store and its data for retry. Null, out-of-range,
stale and atomic-context operations are rejected. Final puts remain nonblocking.

Four new production-C mutations abort: omitted final unpin, omitted redundant
pin release, duplicate canonical heads and deletion with live folios. Existing
completion/bitmap/lockref/sequence/page/folio controls remain rejected. The failed
initial host compile records a real phys_addr_t/uint64_t pointer type mismatch;
the bridge now uses a correctly typed uint64_t temporary and value assignment.
No type checks or sanitizers were weakened. The report layout assertion was
added during host003's controls; its final restored executable was built with
that assertion and run again successfully. No runtime source changed afterward.

94 header-first/repeated combinations, 18 expected configuration/type failures
and four unsupported-operation link failures pass. target002 compiles the final
C adapters with actual pinned native headers. Native completion/VMA C test units
and the version9 userspace client compile against those headers too. The diagnostic
report is112 bytes with one store create/free and two workers/256 iterations.
These are host compiler results, not a native build or kernel runtime test.

The native C++ bridge calls create_area_etc with B_NO_LOCK, then pairs writable
lock_memory_etc/unlock_memory_etc for each pinned page. Actual physical extents
are queried while pinned and validated; failed queries unwind the native pin.
native-api-contract.json records exact source blobs inspected for VM contracts.
The attempted C++ cross compile fails because the matching target C++ <new>
header is unavailable in this host header setup. No substitute header was made.
Native C++ build/export/link checks and runtime validation remain outstanding.

KVM keyboard input still returns USB I/O timeouts. Read-only screenshot
/home/dan/i915-vm/evidence/nanokvm-image-1794602567192972432-0.jpg still shows
the old successful owned-page run. No new code has reached RenkuOS, no new
module is installed, and no native run is claimed. No lease was acquired for
these host changes. Last actual native checkpoint remains physical-pages-001,
version7. Both folio/version8 and this store/version9 runtime work are pending.

This provider is not yet wired into Linux shmem_file_setup, struct file,
address_space or i915 GEM. Real file/mapping ownership, truncate, dirty/accessed,
writeback, GFP constraints and process-mm services are still required. Original
whole i915 compilation remains incomplete at the previous recorded VM/process
boundaries. All118 original Linux imports still verify unchanged.

Reproduce from the repository root:

```sh
SANITIZER_LDFLAGS='-L/home/dan/i915-test-libs/root/usr/lib64 -Wl,-rpath,/home/dan/i915-test-libs/root/usr/lib64' \
  python3 experimental/i915/completion/run-host-tests.py --output NEW_HOST_DIRECTORY
python3 experimental/i915/target-build.py --source /home/dan/renkuos-i915-source
python3 experimental/i915/tests/run-bitops-headers.py --output NEW_HEADER_DIRECTORY
```

Exact target C probe commands and the unsuccessful C++ command are retained.
The full goal remains porting and running i915, not just these CPU tests.
