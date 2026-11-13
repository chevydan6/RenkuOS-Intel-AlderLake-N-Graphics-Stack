<!-- SPDX-License-Identifier: MIT -->
# Native owned-page allocation and lifetime

Actual N100 (GPU8086:46d1 untouched), hrev60072+518, GCC/G++13.3 and SDK518.
Builds/tests ran visibly in fullscreen Terminal through NanoKVM. Four47294-check
completion runs and four650-check VMA runs pass:191776 checks, zero failures,
actual signals and diagnostic unload0. Twenty native area runs were allocated
and actually deleted; orders0–3 were exercised with compound flags. Eight page
users perform2048 reference/map/write/unmap/release iterations. Native tests
compare actual memory-map physical extents/alignment, verify native area removal,
exercise nested original atomic maps and queue final release under IRQ exclusion.
Original scatterlist helpers resolve managed backing, offsets and physical IDs.
These are CPU physical pages, not device DMA addresses or GPU execution.

The C++ bridge uses installed private VM headers and real create_area_etc with
virtual/physical alignment and contiguous wired backing. Native vm.h/vm_types.h
hashes match the pinned RenkuOS source headers; native-sdk-identity.log retains
them and actual exported symbol addresses. Required original native configuration
headers are exported unchanged from4799bb5, with the baseline License.md;
native-config-provenance.json records blobs/hashes. No VM restriction structures
were guessed or copied into a Linux ABI wrapper.

Native directory /boot/home/i915-pages-001, report/clientversion7. Source passed
7102 full checksum entries before/after tests. Runtime bundle002 reused6783
byte-identical references via ../i915-seqlock-001/reference; the full manifest
still verifies those files. Overlays003/004/005 correct a native-only test error
constant and add SDK configuration/platform include paths. Final runtime matches
native-bundle005. All114 original Linux imports remain unchanged. Early failed
compile logs are retained; none loaded a module. Raw archives/bundles are under
/home/dan/i915-vm/pages-port-001; result.json records their hashes. Matching final
binary hashes, original helper hashes, source inventories and final screenshot
are retained here. Known original find_bit warnings keep their prior narrow
warning exception; the new source builds with errors enabled.

Lifecycle and limits are in ../../completion/README.md. The release worker is
joined before unload; this does not implement Linux reclaim, migration, global
page metadata, folios/page cache, or user-copy exception-table semantics.
Whole native i915 attempts still exit1. Physical GEM now passes its highmem
include without importing unused VFS/stat interfaces, then fails on actual Linux
page-flags/folio metadata and generated VM bounds through shmem dependencies.
Ring/GTT still fail on process-mm/task services. No fields/bounds were fabricated.
The overall i915 port remains incomplete.

Final screenshot shows sourcecheck0 and SSH shutdown0. Subsequent SSH was refused,
the test lease released, and media/display baseline preserved; cleanup metadata
records these observations.
