# Shmem ownership implementation plan

Execute the native-ownership extension required by I915_PORT_HANDOFF.md and
the adjacent shmem-ownership design. Work in the isolated sparse checkout on
`codex/i915-shmem-ownership`, based on the exact GitHub commit/tree ed1cdf7.

1. Add focused host lifetime tests for owned-store get/put, closing with live
   folios, final callback, and retry after native deletion failure. Declare the
   contract in native_pages.h, run the test and retain the missing-symbol
   failure. Implement deferred store destruction in src/native_pages.c using
   its existing lock, registry, queue and joined release worker. Existing
   explicit destroy remains available. Run the focused tests with ASan/UBSan.
2. Add Linux-facing file/address_space/shmem tests: setup/EOF/rounding, get/fput,
   zero-fill and retention, concurrent canonical folios, final file close with
   a live folio, mapping GFP constraints and failure unwind. Watch the missing
   API fail, then implement native_shmem.c and scoped fs/file/shmem_fs headers.
   Add real native no-reserve backing via the bridge, retaining existing create
   callers. Add mutation controls for early deletion, lost unpin and lost file
   reference. Run focused tests and the existing completion host regression.
3. Integrate shared shmem cases into the native diagnostic, update the report
   contract and client, and include the new source in native/host builds.
   Build a fresh hash-verified native bundle. Acquire a bounded exclusive test
   lease, show its log in native Terminal, execute through SSH, and retain native
   compile/runtime/unload, component hashes and original whole-source failures.
4. Review the whole change independently, address material findings, rerun the
   relevant checks, and commit the implementation/evidence with sign-off and
   machine-assistance disclosure. Update the handoff without claiming full i915
   or GPU execution. Preserve credentials and raw logs outside the repository.

Verification commands: the new `tests/run-shmem-host-tests.py --output NEW_DIR`,
`completion/run-host-tests.py --output NEW_DIR`, `verify-upstream.py`, the
existing native build/test procedure and original whole-source compiler loop.
Expected results: zero functional errors and actual successful cleanup; genuine
whole-driver dependency errors remain separately recorded.

Review focus: close while a pin is in flight; queued versus in-flight unpin;
delete retry retaining owner; once-only callback outside the spinlock; allocation
mask constraints; zero and near-overflow sizes; unchanged imported source bytes.
