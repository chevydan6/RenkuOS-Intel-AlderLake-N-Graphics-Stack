<!-- SPDX-License-Identifier: MIT -->
# Original CPU-mask helpers with native CPU state

Actual N100, RenkuOS hrev60072+518, installed GCC13.3/SDK518. Native compilation,
kernel tests and complete GEM/ring compiler attempts ran visibly in fullscreen
Terminal via NanoKVM. SSH only transferred files and collected evidence. All 99
imported Linux files remain byte-exact at the pinned v6.6 revision.

Four runs per suite pass with zero failures:

| Suite | Checks/run | Total |
| --- | ---: | ---: |
| CPU masks/cache/helpers + prior bitmap/completion/preemption | 46555 | 186220 |
| Original DRM VMA regression | 648 | 2592 |
| Older native kernel-service regression | 62 | 248 |

All runs report the actual native count 4, online 4, mask 0xf. The adapter queries
smp_get_num_cpus and get_cpu_info; tests independently query the existing native
service again and compare each enabled flag. Original masks, lookup tables,
iteration, set/clear/test, logical operations and inline mask storage are checked.
Native CPUID verifies the selected 64-byte cache alignment. This is not cache
maintenance, DMA/cache-coherence proof, scheduler replacement or GPU work.

The snapshot has a 64-CPU capacity matching the supported native EFI/x86-64
baseline. It rejects oversized or empty state. Possible/present IDs are dense;
online/active exclude disabled IDs. No native CPUSet is cast to a Linux object.
Initialization/reset are serialized, and the owner excludes CPU enabled/topology
changes for the entire lifetime and excludes users during teardown. There are
no CPU hotplug notifications, affinity, cross-CPU calls or NUMA placement services.
This is a bounded adapter, not a production live-topology lifecycle.

Prior cases retain 96 waiter workers, actual SIGUSR1 interruptible result -512
and ordinary-signal killable control result 1949. Actual killing is not tested.
VMA runs retain 48,000 stress operations with overlapping readers; KPI retains
16,000 worker iterations. Each diagnostic unloads with status 0, its driver/link
is removed, and graphics publication is unchanged. Display/owner/GART modules
were preserved. The diagnostic report version changed to 2 with explicit counts
and mask; module/client were compiled together from the verified bundle.

## Source, diagnostics and access state

Physical directory /boot/home/i915-cpumask-002 uses host native-bundle-002 without
an overlay. All 7046 payload entries verify both before and after native tests.
The full archive, manifests/check logs and actual binaries remain outside Git,
identified by result.json/source-checks.json; a short non-reference source list
is committed here. Separate saved completion-build and vma-build snapshots
identify modules even though builds reuse build/. KPI artifacts are build/native.

The original find_bit.c warnings and guarded public nth-entry preconditions from
the previous checkpoint still apply; this is not a warning-free build. Initial
host namespace/header failures remain outside Git in cpumask-port-001; subsequent
44 first/repeated-header controls pass. Native export inspection confirms the
existing CPU query services. New methods do not simulate successful OS work.

Final screenshot shows all suites, full-source failures, final source-check 0 and
SSH-stop 0. SSH2222 subsequently refuses connections and the physical lease was
released. Original custom disk.img remains mounted ro 0/cdrom 0, USB configured.

## Remaining whole original files

All three native compiler attempts still return 1; exact logs are in whole-build.
Ring now reaches generated/bounds.h through Linux VM page-layout definitions.
GTT passes earlier random/limits/CPU/cache issues and reaches asm/spinlock_types.h
for raw architecture lock types. Physical GEM retains MAJOR/MINOR/MKDEV namespace,
__clear_bit_unlock, RCU helper and generated/bounds.h dependencies. No dummy
bounds, page structures, raw locks or RCU lifetime were fabricated to bypass them.
A complete i915 build and GPU execution remain unproven. These are the next actual
layout/service adaptation boundaries, rather than a reason to rewrite GPU code.
