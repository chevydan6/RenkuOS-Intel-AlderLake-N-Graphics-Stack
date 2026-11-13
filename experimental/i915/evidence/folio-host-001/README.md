<!-- SPDX-License-Identifier: MIT -->
# Managed-folio host checkpoint; native validation pending

Host002 passes 47,416 ASan/UBSan checks using the production C adapters and
original completion, lockref, seqcount, refcount, folio helpers and local mapping
bodies. The OS model uses real host RAM, joined pthreads and POSIX semaphores;
physical addresses, IRQ and fault state are explicitly modeled. This is not
native VM, hardware DMA, TSAN or Linux memory-model proof.

Tests cover ordinary order-zero and compound higher-order canonical heads,
const preservation, actual registry shape/order/refcounts, offsets through
original local mappings, concurrent local/atomic map users and full/empty
original folio batches. Batch references are balanced, and final release under
preemption exclusion queues actual backend deletion. Tail-redirection and lost
batch-release mutations abort. Existing allocation/worker failure, deletion,
reference, completion, bitmap, lockref and sequence mutations remain rejected.

Header controls pass 94 first/repeated combinations and 18 expected configuration
or type failures. Actual linking rejects both unsupported BH and exceptional
swap/DAX batch operations in both header modes. Host VMA and DMA regressions
pass, including their mutation controls. target-002 compiles the C components
against the actual pinned Renku headers; completion-probe and vma-probe compile
the native C test units. These are host compiler results, not a RenkuOS build.
The C++ native backing bridge is unchanged from physical-pages-001.

Host002 ran when the import count was 117. The only later import is unchanged
complete GEM shmem C source, added to the whole-source compiler loop, not the
runtime tests. Final target/VMA/DMA checks verify all 118 imports. All 118
manifest blobs were separately checked against their source paths at the
pinned Linux revision and against SHA256 and Git blob bytes.

Whole physical GEM and newly included whole GEM shmem still fail in real Linux
shmem/swap/mmzone dependencies: generated VM bounds are deliberately rejected,
and asm/page.h remains absent. No guessed bounds, zones, page flags or successful
page-cache/writeback placeholders are supplied. Ring/GTT still require actual
process-mm/task integration as recorded at the previous native checkpoint.

Native validation has not occurred for these additions. NanoKVM keyboard Enter
and typing return `timeout after 50ms: write /dev/hidg0: i/o timeout` before
transfer or compilation; custom SSH 2222 is stopped and default SSH recovery
with the existing key timed out. The old successful pages Terminal remains on
screen. No new module was installed. A physical RenkuOS restart was requested.
Last actual native evidence remains physical-pages-001, version7. The new report
version8 expects seven allocations/frees, but no native check count is claimed.

Reproduce from the repository root:

```sh
python3 experimental/i915/completion/run-host-tests.py --output NEW_HOST_DIRECTORY
python3 experimental/i915/target-build.py --source /home/dan/renkuos-i915-source
python3 experimental/i915/tests/run-bitops-headers.py --output NEW_HEADER_DIRECTORY
python3 experimental/i915/vma/run-host-tests.py --output NEW_VMA_DIRECTORY
python3 experimental/i915/dma/run-host-tests.py --output NEW_DMA_DIRECTORY
```

If sanitizer libraries are outside the default paths, set SANITIZER_LDFLAGS
as recorded in the build logs. Target
compiler commands and exact input flags are preserved in the logs. The full
native bundle can be reproduced using completion/prepare-bundle.py with the
pinned Linux and Renku source trees. The whole i915 port remains incomplete.
