<!-- SPDX-License-Identifier: MIT -->
# Host and target-header lockref controls

completion/ is final host003: ASan/UBSan46987 checks, two genuine pthread lockref
workers and8000 iterations, real host signal interruption, original completion/
bitmap controls plus dead-reference mutation and unlocked-mark-dead rejection.
All controls abort as expected. The lost completion-count mutation can now fail
at the lockref fixture's bounded worker lifecycle assertion before the older
fixture; both exact diagnostic forms are accepted with SIGABRT required.

headers/ contains70 first/repeated include controls plus four explicit failures:
VM bounds outside lockref and architecture cmpxchg enabled, host and target.
The bounds shim supplies no layout values. vma/ and dma/ retain passing host
regressions with mutation controls. target-build.log compiles actual native
adapters and the complete original lockref using pinned RenkuOS headers;
vma-target-probe.log compiles the revised fixture using those target headers.
These are cross compilations, not native execution. Bundle logs verify the
completion/VMA/KPI export paths carry the unchanged extracted GFP helper.

Whole-file cross attempts still fail. Ring/GTT reach Linux sched/mm.h and real
process-mm/task services; physical GEM reaches seqcount/seqlock types and
asm/stat.h. No invented process-mm/page layouts are provided. Actual revised
native VMA testing remains pending; see ../physical-lockref-001.
