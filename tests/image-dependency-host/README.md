# Image dependency custody checks

Run `python3 tests/image-dependency-host/run.py /absolute/isolated/output`.
Composes pinned production source 4799bb5a188c4bb6bbef967287640cc21a061a0d
with patches 0100–0103. Extracts the actual image structure and production
get/put dependency, init/uninit and public get/put bodies; no ownership algorithm
is duplicated in the harness. Infrastructure, callbacks, recursive locks and
image search/release are mocked. Child acquisitions use production get_module.

Checks allocation failure, every position in a three-child dependency set,
exact private prefix and empty suffix, unpublished slots, retained parent/child
references, Building reentry, shared root/core leases including callback-nested
root-to-core acquisition, intermediate release, final retained failure, failed
INIT retention, repeated failed get/put and temporary search-pin balance.
Final release quarantines and releases no child: cessation and reference
consumption proof remain unavailable. Failed INIT stage cleanup and unlocked
lifecycle execution remain unresolved. No public ABI change.

ASan/UBSan run with ASAN_OPTIONS=detect_leaks=0 because LeakSanitizer is restricted
under tracing. Mocked locking does not validate concurrent kernel execution.
These are host checks, not native builds or GPU execution. No hardware/VM work.
