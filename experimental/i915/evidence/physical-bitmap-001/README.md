<!-- SPDX-License-Identifier: MIT -->
# Original Linux bitmap searches on native N100

Compilation, kernel execution and complete original GEM/ring compiler attempts
ran visibly in fullscreen RenkuOS Terminal through NanoKVM on the physical N100.
The installed SDK is hrev60072+518 and GCC13.3. SSH only transferred files and
collected evidence. 87 unique imports match the pinned Linux v6.6 Git blobs.
Complete original find_bit.c and hweight.c link into the diagnostic, with original
generic bit-scan and non-atomic helpers. Atomic mutations use compiler atomics.

Four final runs per suite pass, zero failures, actual diagnostic unload status 0:

| Suite | Checks/run | Total |
| --- | ---: | ---: |
| Bitmap/search/helper + completion/preemption | 46389 | 185556 |
| Complete DRM VMA regression | 648 | 2592 |
| Older kernel-service regression | 62 | 248 |

The search oracle walks individual bits independently. It covers empty/partial/
multiple words, all-zero/all-one and patterned maps, dirty trailing bits, offsets
past the limit, first/next/last/nth, zero/AND/AND-NOT/OR searches and inline paths.
All 192 bit positions exercise atomic and non-atomic mutations. These tests do
not establish atomic concurrency stress coverage. Original nth search absence
is tested as >=size: v6.6 can return 64 on a constant partial word despite its
comment saying size. Direct private nth entry with size 0 is unsafe; the public
wrappers reject n>=size before that call and are tested with NULL/size 0 backing.
The port retains these upstream details rather than silently changing algorithms.

Original completion cases retain 96 waiter workers, real interruptible SIGUSR1
result -512, and killable ordinary-signal control result 1949 remaining ticks.
Actual kill is not tested. VMA retains 48,000 stress operations and required
reader overlap. Graphics publication remains unchanged; diagnostic drivers/links
are removed and actual unload 0 is observed for each suite.

## Compiler diagnostics

The original private nth search helper emits four maybe-uninitialized warnings
because its standalone size 0 path has no assigned tmp. Public wrappers guard this
case. Only find_bit.c uses -Wno-error=maybe-uninitialized; the warnings remain in
completion-build-002.log. This is not a warning-free build. An initial strict
build failure is retained in ../bitmap-host-001/initial-strict-find-build.log.
The original find header also triggers -Wextra unsigned-zero comparison warnings
inside GENMASK constant input checks. The wrapper scopes that one warning off
around the header. Invalid GENMASK still fails compilation in the negative control.
No missing function/type/link error is suppressed, and no success stub is added.

## Source and result identity

The physical base is native-bundle-001. Two header-order fixes were overlaid:
include/linux/bits.h loads the base environment before original helpers, and
asm/bitsperlong.h loads architecture config first. Their exact hashes are in
header-update.sha256. All native suites were then rebuilt and rerun; only these
final runs count above. Earlier passing results remain in the external archives.
Final bundle002 contains that same runtime source. A visible checksum check
verifies all 7013 non-Markdown files against it. Full source manifests/check logs,
raw drivers/probes and source archives remain outside Git, identified in
result.json and source-checks.json. The committed short manifests omit reference/
files; reference-provenance.json records their pinned verification.

The build scripts reuse build/, so distinct completion-build-002 and vma-build-002
snapshots identify the final modules separately from build/native's KPI module.
Binaries and their exact SHA256 manifests are preserved in the raw results.
Final screenshot shows successful tests/source check, whole-source compiler
failures and SSH stop 0. Dedicated SSH2222 then refused connections; the physical
lease is released. Original custom disk.img ro 0/cdrom 0 and configured USB remain.
Display/owner/GART modules were preserved; no GPU command or GPU IRQ is tested.

## Current full-source boundary

All original complete GEM/ring compiler attempts still return 1. Ring now passes
the previous bit-scan/bitmap/loff_t/layout errors and reaches cpumask's Linux-UP
num_online_cpus definition conflicting with the actual native service, plus
asm/cache.h. GTT passes asm/ioctl.h and reaches early random/init declarations,
U32_MAX, static-key atomic helpers, cpumask and asm/param.h. Physical GEM still
reaches device-number macros, RCU helpers and asm/cache.h. Actual errors and exits
are in whole-build/. CPU masks, random/static-key services and full lib/bitmap.c
allocation/parsing remain integration work. A complete i915 build, GPU execution
and Mesa support have not been demonstrated.
