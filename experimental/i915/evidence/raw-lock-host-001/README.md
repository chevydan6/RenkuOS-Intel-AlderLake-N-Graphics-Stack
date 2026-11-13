<!-- SPDX-License-Identifier: MIT -->
# Raw lock and bitmap equality host validation

Host ASan/UBSan runs original completion/swait/search and helper code over actual
pthread locks and modeled IRQs; it passes 46742 checks and host signal controls.
Original raw and regular structures are distinct, with host architecture storage.
Public bitmap equality and the directly linked original helper are checked with
an independent bit-by-bit oracle across empty/partial/multiword maps and dirty
trailing bits. Lost completion-count, ignored search offset and ignored equality
tail-mask mutations all abort. Invalid GENMASK, build_bug and mixed-min compiler
controls reject their intended errors; the positive helper compiles.

The initial equality link failure is retained as initial-missing-bitmap-equality-link.log. extract-bitmap.py verifies the full imported lib/bitmap.c hash and emits
only the complete unchanged __bitmap_equal function, plus its provenance hash.
Other external bitmap allocation/parsing/helpers are not supplied by that slice.
Original find_bit warnings and private nth-entry preconditions still apply.

The IRQ model compiles actual native adapters with target declarations and models
native services. It checks actual original bit-spin methods' state transitions
and both unlocks; premature IRQ-enable mutation exits 90. It is serialized, not
hardware/concurrency proof. Native concurrency results are recorded separately.
CPU, VMA and DMA sanitizer/model controls and prior component/target probes pass.

Fifty-four first/repeated-header controls compile with host and real target
headers. Nineteen additional native-source objects, including new driver cases,
compile against target headers as recorded in native-source-probes-001. These
cross-source object probes do not constitute native linking or kernel execution;
actual native compilation/runtime is in physical-raw-lock-001.
