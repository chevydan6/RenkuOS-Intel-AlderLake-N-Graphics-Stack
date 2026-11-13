<!-- SPDX-License-Identifier: MIT -->
# Owned-page host controls

Final host004 passes47231 ASan/UBSan checks, real pthread users and the production
C ownership/queue logic. page_bridge_host.c is an explicit OS model using real
host RAM, POSIX semaphores and joined pthreads. Physical IDs are synthetic model
metadata, not native/DMA evidence. Fault allowance/preemption is modeled; actual
VM allocation, kernel fault-control exports and IRQ behavior are validated in
physical-pages-001. The native-only missing-area status assertion was corrected
after host004; its excluded host branch does not affect these results.

Tests cover event/worker/allocation failure cleanup, compound references, blocked
native-release modeling, drain timeout and busy shutdown, stale descriptor lookup,
nesting and zeroed RAM, plus original scatterlist buffer/physical helpers.
The original refcount implementation and extracted complete kmap_atomic helpers
are compiled, not replaced. Independent live-resource accounting rejects missing
reference release and missing VM deletion. Prior completion/bitmap/lockref/sequence
mutations still fail as expected. This is not TSAN or a formal Linux memory-model
proof. No successful user-copy stub or fake native physical map is supplied.

headers/ contains90 first/repeated include controls,14 expected configuration/
type failures (including highmem/local slots and passing a page to kunmap_atomic),
and two unresolved-BH link controls. Host DMA API rollback/mode regressions pass.
target-001.log compiles the C adapters against real pinned native headers;
native-test-probe.log compiles the corrected native test cases. The C++ bridge
was compiled/linked by actual G++ using installed native private headers, as
recorded in the physical evidence. whole-phys-001.log retains cross compiler
failures in unported page-flags/folio interfaces and VM bounds.
