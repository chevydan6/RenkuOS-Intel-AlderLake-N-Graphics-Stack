# Decisions

D1: Astra owns gpu-next-worktree production code and integration. Existing Terra owns /home/dan/renkuos-terra-n100 research/review. Do not launch a duplicate agent.
D2: shared records live here; canonical existing resource locks remain in /home/dan/haiku-development/coordination/locks. Local QEMU is user-authorized.
D3: accepted Terra R1. Exactly one manager lock acquisition must protect lookup through last use; get_aperture is now caller-locked. Actual helper must be included in lifetime fixtures. Independent review and native validation required before integration.
D4: retained execution-module design remains proposed. No GPU workload until memory and hardware recovery gates are satisfied.

## Terra proposal D-T1 — direct-ring MI proof

Astra accepts for first-execution design: use direct-ring MI stores before a separate
PPGTT batch for the first execution proof. Linux LRC selftest is source evidence.
This reduces workload structures without removing context/VM/lifetime requirements.
Follow with immutable PPGTT batch to validate useful address translation.

## Terra review D-T2 — retained owner module

A-F1 source mechanism verified independently. Accept as a candidate architecture,
conditional on explicit shared-aperture leases and no pointers into unloadable
legacy driver state/code. Full implementation design remains for Astra to supply.

## Memory-only owner implementation gate

Astra accepts Terra's latest T3 design disposition: begin isolated owner/lease
implementation without any GPU publication path. Preserve all stated conditions,
especially failed-allocation admission closure, retained-display no-reopen behavior,
sole mapping ownership and permanent record lifetime. A future hardware publication
transition needs separate serialization across actual MMIO publication, not merely
a state check before unlocking. No physical or execution readiness is implied.

## Alternate-kernel boot gate

Accepted Terra read-only loader audit: loader.cpp loads system/kernel_x86_64 via
BootVolume.SystemDirectory; vfs.cpp selects packagefs root from boot system package.
Do not assume non-packaged driver precedence applies to the kernel. Next disposable
boot requires a verified replacement boot system package or another proven loader
mechanism, exact candidate identity, bounded serial/QMP observation and preserved
source040 image. No package activation or candidate boot has occurred yet.

## Proposed native IRQ drain experiment

Design: workstreams/gpu-next/native-irq-drain-design.txt. Use isolated test-only
vector-state instrumentation rather than assuming allocation clears no_lock_vector.
Retain allocation until complete dispatcher return, beyond callback removal.
Pinned user ioctl workers use libgnu affinity; callback has independent time and
iteration limits. No synthetic no-lock race. Terra review requested before code.
This directly validates a GPU callback ownership prerequisite; no GPU workloads.

## Retrace semaphore scope

Accepted Terra source finding: owner exit/exec invalidates public retrace ID;
kernel ownership alone breaks user acquisition and cached-ID behavior. Keep ABI
unchanged pending complete design. Source checks show ID validation, not a
demonstrated UAF; wrap remains untested. First diagnostic completion stays in the
retained worker/polling design, never the public retrace semaphore. All actual
GPU memory/callback/reset/physical gates remain. Native reproducer in progress.

## Withhold normal retrace semaphore cleanup
Accepted Terra's no-lock callback-lifetime objection. Do not treat private
interrupt_handler_installed=false after successful removal as a general drain.
No-lock removal can free the kernel traversal node while dispatch uses it; tested
under a deterministic host model. Driver-local counters or semaphore retention
alone do not solve it. Review a kernel-enforced lifetime contract before integration.
Documented options in research/n100-submission/IRQ-TEARDOWN-CONTRACT.txt; no new
kernel ABI or production patch approved by this checkpoint.

## User authorization: physical test crash risk accepted
Dan explicitly removed the prerequisite to verify recovery access before
controlled physical tests, stating system crashes are acceptable. Do not ask
for that confirmation again. Connectivity is still required to deploy/run tests.
Preserve known-working images, bounded execution, and truthful hardware evidence.
This does not authorize destructive disk writes or removal of memory-safety
prerequisites for GPU command publication. Proceed with controlled hardware
validation when target access is available.

## User visibility requirement: NanoKVM development console
Dan wants to watch actual driver development on NanoKVM. Run target builds,
tests and debugging in the visible RenkuOS terminal through NanoKVM keyboard
control. Keep commands and results visible; do not substitute a decorative
status window for the actual work. Use SSH for necessary transfers/log collection
and explain those actions. Fullscreen Terminal via Alt+Enter verified; Alt+N
opens a new terminal. This applies to future physical development sessions.

A26 — controlled-boot observation scope
Accepted Terra's module lifecycle, lock, and PMCSR-overlap findings. v1 acquires
and permanently retains v0, sharing its existing singleton. PCI calls are outside
the owner mutex; one numeric Request plus operation pin spans admission through
retirement. Read uncertainty, changed ownership or changed snapshot blocks later
observations for this boot. Outputs are private kernel data, never accepted user
pointers/BDFs. No GT MMIO is exposed. This is not a global power hold: firmware,
raw PCI/ACPI users and shutdown are outside it. A real transition barrier and DMA
quiescence remain gates before execution. New mappings/accessors must check their
own register extent; PCI helper trusts owner size metadata without probing BARs.
A26 changes are split into pure-helper and owner/fixture commits for review.

A27: accept explicit UC on same boot-time owner BAR and narrow v2 raw ACK
inventory. GART already maps same ALD BAR strongly UC; weak owner mapping alone
would not establish this. Preserve v0/v1 layout. Fixed two32-bit130044 reads with
read barriers and private Reading phase; final PCI/state recheck before output.
No GT stores or request-bit ownership inference. Invariant breach retains pin.
Terra reviewed source,26 independent host mocks and native artifact hashes.
