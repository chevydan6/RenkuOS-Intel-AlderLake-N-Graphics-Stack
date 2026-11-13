<!-- SPDX-License-Identifier: MIT -->
# Demonstrated blockers
1. Shell git clone cannot resolve github.com. GitHub connector can read/write
   the experimental branch, so repository preservation remains possible.
2. Cached RenkuOS source 5720e2603d2f23fd2e4d6b45b8952825cdf0267c is sparse,
   not a matching installed development SDK. KernelExport.h compilation fails
   at SupportDefs.h; see evidence/002 and 005. Host is Fedora Linux.
3. Complete original intel_ring.c fails at gem/i915_gem_internal.h (001).
   Selected functions compile after explicit extraction, without establishing
   the entire GEM dependency closure.
4. nm confirms unresolved real wait/timeline/panic symbols. No native module
   linkage or functioning GPU wait is demonstrated.
