<!-- SPDX-License-Identifier: MIT -->
# Sequence-lock host controls

Final host002 passes47011 checks under ASan/UBSan, actual pthread workers and
real host signal interruption. Full original seqlock.h is compiled through a
copy of the production wrapper with only its import path redirected to the
exclusive test copy. Removing the original sequence comparison (always accepting
stale reads) must abort; seq-negative-control.log preserves that rejection.
Prior completion/bitmap/lockref mutation and context controls still pass.

An initial host build caught an object-macro collision: preemptible expanded
inside original seqcount generic property names. The public adapter now uses
the function-like macro expected by Linux; raw/spin/mutex-associated count
operations compile and their supported paths are exercised. Initial failure
artifacts remain outside Git in seqlock-port-001/host-001.

headers/ retains80 first/repeated-include combinations, eight expected failures
for unsupported VM bounds/cmpxchg/KCSAN/RT configurations, and two actual link
failures for unimplemented BH masking methods. These methods have private
unresolved symbols; no successful softirq no-op was added. Original disabled
KCSAN/lockdep instrumentation is not runtime race detection.

target-002.log compiles native adapters against pinned real RenkuOS headers;
whole-phys-001.log is a cross compiler attempt advancing past sequence types to
asm/stat.h. These supplement actual physical-seqlock-001, not replace it.
Tests use GNU C/kernel access conventions on x86-64 ordinary RAM; ASan/UBSan
are not a formal Linux memory-model proof or TSAN validation.
